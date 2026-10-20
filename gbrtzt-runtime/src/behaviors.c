/* SPDX-License-Identifier: MIT */
#include <drivers/behavior.h>
#include <gbrtzt/runtime.h>
static int macro_down(struct zmk_behavior_binding *b,struct zmk_behavior_binding_event e){return b->param1<GBR_SLOTS?gbr_macro_start(b->param1,e):-EINVAL;}
static int macro_up(struct zmk_behavior_binding *b,struct zmk_behavior_binding_event e){return 0;}
static int dance_down(struct zmk_behavior_binding *b,struct zmk_behavior_binding_event e){return b->param1<GBR_SLOTS?gbr_dance_event(b->param1,e,true):-EINVAL;}
static int dance_up(struct zmk_behavior_binding *b,struct zmk_behavior_binding_event e){return b->param1<GBR_SLOTS?gbr_dance_event(b->param1,e,false):-EINVAL;}
#if IS_ENABLED(CONFIG_ZMK_BEHAVIOR_METADATA)
static const struct behavior_parameter_value_metadata values[]={{.display_name="Slot",.range={.min=0,.max=GBR_SLOTS-1},.type=BEHAVIOR_PARAMETER_VALUE_TYPE_RANGE}};
static const struct behavior_parameter_metadata_set sets[]={{.param1_values=values,.param1_values_len=1}};
static const struct behavior_parameter_metadata metadata={.sets=sets,.sets_len=1};
#define GBR_METADATA .parameter_metadata=&metadata,
#else
#define GBR_METADATA
#endif
static const struct behavior_driver_api macro_api={.binding_pressed=macro_down,.binding_released=macro_up,GBR_METADATA};
static const struct behavior_driver_api dance_api={.binding_pressed=dance_down,.binding_released=dance_up,GBR_METADATA};
BEHAVIOR_DT_DEFINE(DT_NODELABEL(gbr_macro),NULL,NULL,NULL,NULL,POST_KERNEL,CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,&macro_api);
BEHAVIOR_DT_DEFINE(DT_NODELABEL(gbr_dance),NULL,NULL,NULL,NULL,POST_KERNEL,CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,&dance_api);
