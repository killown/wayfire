#include <wayfire/output-layout.hpp>

namespace wf
{
namespace layout_detail
{
void priv_output_layout_fini(wf::output_layout_t *layout);
std::string_view get_output_source_name(output_image_source_t source);
std::string wl_transform_to_string(wl_output_transform transform);
}
}
