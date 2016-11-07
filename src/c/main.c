#include "pebble.h"
#include "main.h" // Used for drawing hands
#include "gcolor_definitions.h" // Allows the use of colors such as "GColorMidnightGreen"

static Window *s_main_window; 
static Layer *s_solid_bg_layer, *s_hands_layer;
static BitmapLayer *s_background_layer; 
static GBitmap *s_background_bitmap; 
static GPath *s_minute_arrow, *s_hour_arrow, *s_second_arrow; 
static TextLayer *s_date_layer;

static void bg_update_proc(Layer *layer, GContext *ctx) {
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone); 
}

static void hands_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer); 
  
  time_t now = time(NULL); 
  struct tm *t = localtime(&now); 
  
  static char s_buffer_date[8]; 
  strftime(s_buffer_date, sizeof("dd/MMM/yy"), PBL_IF_ROUND_ELSE("%d", "%b %d"), t);
  // Display this date on the TextLayer
  text_layer_set_text(s_date_layer, s_buffer_date);
  
  // Hour hand
  graphics_context_set_fill_color(ctx, GColorWhite); 
  graphics_context_set_stroke_color(ctx, GColorBlack); 
  gpath_rotate_to(s_hour_arrow, (TRIG_MAX_ANGLE * (((t->tm_hour % 12) * 6) + (t->tm_min / 10))) / (12 * 6)); 
  gpath_draw_filled(ctx, s_hour_arrow); 
  gpath_draw_outline(ctx, s_hour_arrow); 
  
  // Minute hand
  graphics_context_set_fill_color(ctx, GColorWhite); 
  gpath_rotate_to(s_minute_arrow, TRIG_MAX_ANGLE * t->tm_min / 60);
  gpath_draw_filled(ctx, s_minute_arrow); 
  gpath_draw_outline(ctx, s_minute_arrow);
  
  //Second hand
  graphics_context_set_fill_color(ctx, GColorRed); 
  gpath_rotate_to(s_second_arrow, (TRIG_MAX_ANGLE * t->tm_sec / 60)); 
  gpath_draw_filled(ctx, s_second_arrow); 
  graphics_context_set_stroke_color(ctx, GColorRed); 
  gpath_draw_outline(ctx, s_second_arrow); 
  
  // Center dot
  graphics_context_set_fill_color(ctx, GColorBlack); 
  graphics_fill_rect(ctx, GRect(bounds.size.w / 2 - 1, bounds.size.h / 2 - 1, 3, 3), 0, GCornerNone); 
  GPoint pos = GPoint(bounds.size.w / 2, bounds.size.h / 2);
  graphics_fill_circle(ctx, pos, 5);
  graphics_context_set_stroke_color(ctx, GColorWhite); 
  graphics_draw_circle(ctx, pos, 5);
  
}

static void handle_second_tick(struct tm *tick_time, TimeUnits units_changed) {
  layer_mark_dirty(window_get_root_layer(s_main_window));
  
}

static void window_load(Window *s_main_window) {
  Layer *window_layer = window_get_root_layer(s_main_window); 
  GRect bounds = layer_get_bounds(window_layer); 
  
  s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_BACKGROUND); 

  s_solid_bg_layer = layer_create(bounds); 
  s_background_layer = bitmap_layer_create(bounds); 
  s_hands_layer = layer_create(bounds); 
  
  layer_set_update_proc(s_solid_bg_layer, bg_update_proc); 
  bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap); 
  layer_set_update_proc(s_hands_layer, hands_update_proc); 
  
  #if PBL_PLATFORM_BASALT // Only set this for SDK 3.0 +
    bitmap_layer_set_compositing_mode(s_background_layer, GCompOpSet); // Set background layer to be transparent
  #endif
  
  layer_add_child(window_layer, s_solid_bg_layer); 
  layer_add_child(window_layer, bitmap_layer_get_layer(s_background_layer)); 
  layer_add_child(window_layer, s_hands_layer); 
  
  // Create date DateLayer
  s_date_layer = text_layer_create(GRect(PBL_IF_ROUND_ELSE(-30,0), PBL_IF_ROUND_ELSE(130, 0), bounds.size.w, 50));
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_text_color(s_date_layer, GColorWhite);
  
  text_layer_set_font(s_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(s_date_layer, PBL_IF_ROUND_ELSE(GTextAlignmentCenter,GTextAlignmentLeft));
  layer_add_child(window_layer, text_layer_get_layer(s_date_layer));
}

static void window_unload(Window *s_main_window) {
  layer_destroy(s_solid_bg_layer); 
  gbitmap_destroy(s_background_bitmap); 
  bitmap_layer_destroy(s_background_layer);
  layer_destroy(s_hands_layer); 
  text_layer_destroy(s_date_layer);
}

static void init() {
  s_main_window = window_create(); 
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = window_load, 
    .unload = window_unload, 
  });
  window_stack_push(s_main_window, true); 

  s_minute_arrow = gpath_create(&MINUTE_HAND_POINTS); 
  s_hour_arrow = gpath_create(&HOUR_HAND_POINTS); 
  s_second_arrow = gpath_create(&SEC_POINTS); 

  Layer *window_layer = window_get_root_layer(s_main_window); 
  GRect bounds = layer_get_bounds(window_layer); 
  GPoint center = grect_center_point(&bounds); 
  gpath_move_to(s_minute_arrow, center); 
  gpath_move_to(s_hour_arrow, center); 
  gpath_move_to(s_second_arrow, center); 

  tick_timer_service_subscribe(SECOND_UNIT, handle_second_tick); 
}

static void deinit() {
  
  gpath_destroy(s_minute_arrow);
  gpath_destroy(s_hour_arrow); 
  gpath_destroy(s_second_arrow); 
  tick_timer_service_unsubscribe(); 
  window_destroy(s_main_window); 
}

int main() {
  init();
  app_event_loop();
  deinit();
}
