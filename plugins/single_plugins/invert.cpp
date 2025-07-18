#include <wayfire/per-output-plugin.hpp>
#include <wayfire/output.hpp>
#include <wayfire/opengl.hpp>
#include <wayfire/render-manager.hpp>

static const char *vertex_shader =
    R"(
#version 100

attribute highp vec2 position;
attribute highp vec2 uvPosition;

varying highp vec2 uvpos;

void main() {

    gl_Position = vec4(position.xy, 0.0, 1.0);
    uvpos = uvPosition;
}
)";

static const char *fragment_shader =
    R"(
#version 100

varying highp vec2 uvpos;
uniform sampler2D smp;
uniform bool preserve_hue;

void main()
{
    highp vec4 tex = texture2D(smp, uvpos);

    if (preserve_hue)
    {
        highp float hue = tex.a - min(tex.r, min(tex.g, tex.b)) - max(tex.r, max(tex.g, tex.b));
        gl_FragColor = hue + tex;
    } else
    {
        gl_FragColor = vec4(1.0 - tex.r, 1.0 - tex.g, 1.0 - tex.b, 1.0);
    }
}
)";

class wayfire_invert_screen : public wf::per_output_plugin_instance_t
{
    wf::post_hook_t hook;
    wf::activator_callback toggle_cb;
    wf::option_wrapper_t<bool> preserve_hue{"invert/preserve_hue"};

    bool active = false;
    OpenGL::program_t program;

    wf::plugin_activation_data_t grab_interface = {
        .name = "invert",
        .capabilities = 0,
    };

  public:
    void init() override
    {
        if (!wf::get_core().is_gles2())
        {
            const char *render_type =
                wf::get_core().is_vulkan() ? "vulkan" : (wf::get_core().is_pixman() ? "pixman" : "unknown");
            LOGE("invert: requires GLES2 support, but current renderer is ", render_type);
            return;
        }

        wf::option_wrapper_t<wf::activatorbinding_t> toggle_key{"invert/toggle"};

        hook = [=] (wf::auxilliary_buffer_t& source,
                    const wf::render_buffer_t& destination)
        {
            render(source, destination);
        };

        toggle_cb = [=] (auto)
        {
            if (!output->can_activate_plugin(&grab_interface))
            {
                return false;
            }

            if (active)
            {
                output->render->rem_post(&hook);
            } else
            {
                output->render->add_post(&hook);
            }

            active = !active;

            return true;
        };

        wf::gles::run_in_context([&]
        {
            program.set_simple(OpenGL::compile_program(vertex_shader, fragment_shader));
        });

        output->add_activator(toggle_key, &toggle_cb);
    }

    void render(wf::auxilliary_buffer_t& source, const wf::render_buffer_t& destination)
    {
        static const float vertexData[] = {
            -1.0f, -1.0f,
            1.0f, -1.0f,
            1.0f, 1.0f,
            -1.0f, 1.0f
        };

        static const float coordData[] = {
            0.0f, 0.0f,
            1.0f, 0.0f,
            1.0f, 1.0f,
            0.0f, 1.0f
        };

        wf::gles::run_in_context([&]
        {
            wf::gles::bind_render_buffer(destination);
            program.use(wf::TEXTURE_TYPE_RGBA);
            GL_CALL(glBindTexture(GL_TEXTURE_2D, wf::gles_texture_t::from_aux(source).tex_id));
            GL_CALL(glActiveTexture(GL_TEXTURE0));

            program.attrib_pointer("position", 2, 0, vertexData);
            program.attrib_pointer("uvPosition", 2, 0, coordData);
            program.uniform1i("preserve_hue", preserve_hue);

            GL_CALL(glDisable(GL_BLEND));
            GL_CALL(glDrawArrays(GL_TRIANGLE_FAN, 0, 4));
            GL_CALL(glEnable(GL_BLEND));
            GL_CALL(glBindTexture(GL_TEXTURE_2D, 0));

            program.deactivate();
        });
    }

    void fini() override
    {
        if (active)
        {
            output->render->rem_post(&hook);
        }

        wf::gles::run_in_context_if_gles([&]
        {
            program.free_resources();
        });

        output->rem_binding(&toggle_cb);
    }
};

DECLARE_WAYFIRE_PLUGIN(wf::per_output_plugin_t<wayfire_invert_screen>);
