#ifndef OUTPUT_HPP
#define OUTPUT_HPP

#include "wayfire/geometry.hpp"
#include "wayfire/object.hpp"
#include "wayfire/bindings.hpp"
#include "wayfire/scene-input.hpp"
#include "wayfire/scene.hpp"
#include "wayfire/signal-provider.hpp"

#include <wayfire/nonstd/wlroots.hpp>
#include <wayfire/option-wrapper.hpp>
#include <wayfire/config/types.hpp>

namespace wf
{
class view_interface_t;
}

using wayfire_view = nonstd::observer_ptr<wf::view_interface_t>;

namespace wf
{
class render_manager;
class workspace_manager;

struct plugin_grab_interface_t;
using plugin_grab_interface_uptr = std::unique_ptr<plugin_grab_interface_t>;

/**
 * Flags which can be passed to wf::output_t::activate_plugin() and
 * wf::output_t::can_activate_plugin().
 */
enum plugin_activation_flags_t
{
    /**
     * Activate the plugin even if input is inhibited, for ex. even when a
     * lockscreen is active.
     */
    PLUGIN_ACTIVATION_IGNORE_INHIBIT = (1 << 0),
    /*
     * Allow the same plugin to be activated multiple times.
     * The plugin will also have to be deactivated as many times as it has been
     * activated.
     */
    PLUGIN_ACTIVATE_ALLOW_MULTIPLE   = (1 << 1),
};

class output_t : public wf::object_base_t, public wf::signal::provider_t
{
  public:
    /**
     * The wlr_output that this output represents
     */
    wlr_output *handle;

    /**
     * The render manager of this output
     */
    std::unique_ptr<render_manager> render;

    /**
     * The workspace manager of this output
     */
    std::unique_ptr<workspace_manager> workspace;

    /**
     * Get a textual representation of the output
     */
    std::string to_string() const;

    /**
     * Get the logical resolution of the output, i.e if an output has mode
     * 3860x2160, scale 2 and transform 90, then get_screen_size will report
     * that it has logical resolution of 1080x1920
     */
    virtual wf::dimensions_t get_screen_size() const = 0;

    /**
     * Same as get_screen_size() but returns a wf::geometry_t with x,y = 0
     */
    wf::geometry_t get_relative_geometry() const;

    /**
     * Returns the output geometry as the output layout sees it. This is
     * typically the same as get_relative_geometry() but with meaningful x and y
     */
    wf::geometry_t get_layout_geometry() const;

    /**
     * Moves the pointer so that it is inside the output
     *
     * @param center If set to true, the pointer will be centered on the
     *   output, regardless of whether it was inside before.
     */
    void ensure_pointer(bool center = false) const;

    /**
     * Gets the cursor position relative to the output
     */
    wf::pointf_t get_cursor_position() const;

    virtual std::shared_ptr<wf::scene::output_node_t> node_for_layer(
        wf::scene::layer layer) const = 0;

    /**
     * Get the workspace set of the output. This is a floating node which contains
     * all the regular views of an output in the WORKSPACE layer.
     */
    virtual scene::floating_inner_ptr get_wset() const = 0;

    /**
     * Checks if a plugin can activate. This may not succeed if a plugin
     * with the same abilities is already active or if input is inhibited.
     *
     * @param flags A bitwise OR of plugin_activation_flags_t.
     *
     * @return true if the plugin is able to be activated, false otherwise.
     */
    virtual bool can_activate_plugin(wf::plugin_grab_interface_t *owner, uint32_t flags = 0) = 0;

    /**
     * Same as can_activate_plugin(plugin_grab_interface_uptr), but checks for
     * any plugin with the given capabilities.
     *
     * @param caps The capabilities to check.
     * @param flags A bitwise OR of plugin_activation_flags_t.
     */
    virtual bool can_activate_plugin(uint32_t caps, uint32_t flags = 0) = 0;

    /**
     * Activates a plugin. Note that this may not succeed, if a plugin with the
     * same abilities is already active. However the same plugin might be
     * activated twice.
     *
     * @param flags A bitwise OR of plugin_activation_flags_t.
     *
     * @return true if the plugin was successfully activated, false otherwise.
     */
    virtual bool activate_plugin(wf::plugin_grab_interface_t *owner, uint32_t flags = 0) = 0;

    /**
     * Deactivates a plugin once, i.e if the plugin was activated more than
     * once, only one activation is removed.
     *
     * @return true if the plugin remains activated, false otherwise.
     */
    virtual bool deactivate_plugin(wf::plugin_grab_interface_t *owner) = 0;

    /**
     * Send cancel to all active plugins,
     * see plugin_grab_interface_t::callbacks.cancel
     */
    virtual void cancel_active_plugins() = 0;

    /**
     * @return true if a grab interface with the given name is activated, false
     *              otherwise.
     */
    virtual bool is_plugin_active(std::string owner_name) const = 0;

    /**
     * Call a plugin's registered activator binding.
     *
     * @param activator The name of the activator binding, for ex. "expo/toggle"
     * @param data The activator data to pass to the view.
     *   Supports also custom data types.
     *
     * @return True if a plugin's binding matches the given name and the plugin
     *   consumes the event, False otherwise.
     */
    virtual bool call_plugin(const std::string& activator,
        const wf::activator_data_t& data) const = 0;

    /**
     * @return The topmost view in the workspace layer
     */
    wayfire_view get_top_view() const;

    /**
     * Get the most recently focused view on this output.
     *
     * Note that the view might not be actually focused, as focus can be overridden
     * by core, layer-shell views or plugins.
     */
    virtual wayfire_view get_active_view() const = 0;

    /**
     * Set the view as the output's active view.
     *
     * This operation will change the view's last_focus_timestamp and its activated
     * status. In addition, an attempt to focus the view on the current seat will
     * be made. Note, however, that the last operation may fail if layer-shell
     * views, plugin grabs or something similar overrides the focus request.
     *
     * @param raise If set to true, the view will additionally be raised to the
     *   top of its layer.
     */
    virtual void focus_view(wayfire_view v, bool raise = false) = 0;

    /**
     * Try to focus the given scenegraph node. This may not work if another node
     * requests a higher focus_importance.
     *
     * Note that the focus_view function should be used for view nodes, as
     * focusing views typically involves more operations. Calling this function
     * does not change the active view on the output, even if the newly focused
     * node is a view node!
     *
     * The new_focus' last focus timestamp will be updated.
     */
    virtual void focus_node(wf::scene::node_ptr new_focus) = 0;

    /**
     * Get the last focus timestamp which was given out by this output.
     */
    virtual uint64_t get_last_focus_timestamp() const = 0;

    /**
     * Switch the workspace so that view becomes visible.
     * @return true if workspace switch really occurred
     */
    bool ensure_visible(wayfire_view view);

    /**
     * Trigger a refocus operation on the output.
     * See scene::node_t::keyboard_refocus() for details.
     */
    virtual void refocus() = 0;

    /**
     * the add_* functions are used by plugins to register bindings. They pass
     * a wf::option_t, which means that core will always use the latest binding
     * which is in the option.
     *
     * Adding a binding happens on a per-output basis. If a plugin registers
     * bindings on each output, it will receive for ex. a keybinding only on
     * the currently focused one.
     *
     * @return The wf::binding_t which can be used to unregister the binding.
     */
    virtual wf::binding_t *add_key(option_sptr_t<keybinding_t> key,
        wf::key_callback*) = 0;
    virtual wf::binding_t *add_axis(option_sptr_t<keybinding_t> axis,
        wf::axis_callback*) = 0;
    virtual wf::binding_t *add_button(option_sptr_t<buttonbinding_t> button,
        wf::button_callback*) = 0;
    virtual wf::binding_t *add_activator(option_sptr_t<activatorbinding_t> activator,
        wf::activator_callback*) = 0;

    /**
     * Remove the given binding, regardless of its type.
     */
    virtual void rem_binding(wf::binding_t *binding) = 0;

    /**
     * Remove all bindings which have the given callback, regardless of the type.
     */
    virtual void rem_binding(void *callback) = 0;

    virtual ~output_t();

  protected:
    /* outputs are instantiated internally by core */
    output_t();
};
}

#endif /* end of include guard: OUTPUT_HPP */
