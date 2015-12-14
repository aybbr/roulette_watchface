#include <pebble.h>
#include <time.h>

#define COLORS       true
#define ANTIALIASING true

#define HAND_MARGIN  15
#define FINAL_RADIUS 75

#define DC_SIZE 70

#define ANIMATION_DURATION 500
#define ANIMATION_DELAY    600
static int32_t s_minutes;
//static TextLayer *s_day_layer;
static GBitmap *s_example_bitmap;
static BitmapLayer *s_bitmap_layer;
typedef struct {
  int hours;
  int minutes;
} Time;
//static WeekDay wd;
static Window *s_main_window;
static Layer *s_canvas_layer;
static Layer *s_hands_layer;

//static Layer *s_c_arc;
//
//static Layer      *date_layer;
//static TextLayer  *date_text_layer;

static GPoint s_center;
static GPoint s_mini_cercle_center;
static GPoint s_hour_center;
static Time s_last_time, s_anim_time;
static int s_radius = 0, s_anim_hours_60 = 0, s_color_channels[3];
static int diamsmall = 20;
static bool s_animating = false;
static GPath *s_my_path_ptr = NULL;
static TextLayer *time_layer_h;
static TextLayer *time_layer_m;
static int day_border_1, day_border_2;

GRect start;
static const GPathInfo BOLT_PATH_INFO = {
  .num_points = 7,
  .points = (GPoint []) {{21, 0}, {14, 26}, {28, 26}, {7, 60}, {14, 34}, {0, 34}, {0, 8}}
};
static int32_t get_angle_for_minute(int minute) {
  // Progress through 60 minutes, out of 360 degrees
  return (minute * 360) / 60;
}
 

// .update_proc of my_layer:
void my_layer_update_proc(Layer *my_layer, GContext* ctx) {
  

  
  // Fill the path:
  graphics_context_set_fill_color(ctx, GColorWhite);
  gpath_draw_filled(ctx, s_my_path_ptr);
  // Stroke the path:
  graphics_context_set_stroke_color(ctx, GColorOxfordBlue);
  gpath_draw_outline(ctx, s_my_path_ptr);
  
}

void setup_my_path(void) {
  s_my_path_ptr = gpath_create(&BOLT_PATH_INFO);
  // Rotate 15 degrees:
  gpath_rotate_to(s_my_path_ptr, TRIG_MAX_ANGLE / 360 * 15);
  // Translate by (5, 5):
  gpath_move_to(s_my_path_ptr, GPoint(5, 5));
}
/*************************** AnimationImplementation **************************/

static void animation_started(Animation *anim, void *context) {
  s_animating = true;
}

static void animation_stopped(Animation *anim, bool stopped, void *context) {
  s_animating = false;
}

static void animate(int duration, int delay, AnimationImplementation *implementation, bool handlers) {
  Animation *anim = animation_create();
  animation_set_duration(anim, duration);
  animation_set_delay(anim, delay);
  animation_set_curve(anim, AnimationCurveEaseInOut);
  animation_set_implementation(anim, implementation);
  if(handlers) {
    animation_set_handlers(anim, (AnimationHandlers) {
      .started = animation_started,
      .stopped = animation_stopped
    }, NULL);
  }
  animation_schedule(anim);
}

/************************************ UI **************************************/

// Called every minute
static void update_time() {
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);
  
  s_last_time.minutes = tick_time->tm_min;
  s_last_time.hours = tick_time->tm_hour;

  // Create a long-lived buffer
  static char buffer_h[] = "00";
  static char buffer_m[] = "00";
  static char num_day[] = "00";
  
  // Write the current hours and minutes into the buffer
  if(clock_is_24h_style() == true) {
    // Use 24 hour format
    strftime(buffer_h, sizeof("00"), "%H", tick_time);
    strftime(buffer_m, sizeof("00"), "%M", tick_time);
  } else {
    // Use 12 hour format
    strftime(buffer_h, sizeof("00"), "%I", tick_time);
    strftime(buffer_m, sizeof("00"), "%M", tick_time);
  }

  // Display this time on the TextLayer
  text_layer_set_text(time_layer_h, buffer_h);
  text_layer_set_text(time_layer_m, buffer_m);
  // get current day of the week lundi : 340, 32 | tue : 
   strftime(num_day, sizeof("00"), "%w", tick_time);
  int i = num_day[0];
  switch(i) {
      case '0' :
         day_border_1= -20;
         day_border_2= 285;
         break;
      case '1' :
     day_border_1= 35;
     day_border_2= 340;
        break;
      case '2' :
         day_border_1= -273;
         day_border_2= 45;
         break;
      case '3' :
        day_border_1= -215;
         day_border_2= 92;
         break;
      case '4' :
        day_border_1= -163;
         day_border_2= 148;
         break;
     case '5' :
        day_border_1= -123;
         day_border_2= 193;
         break;
     case '6' :
        day_border_1= -80;
         day_border_2= 230;
         break;
      default :
         APP_LOG(APP_LOG_LEVEL_WARNING, "invalid weekday :%s ",num_day);
   }
}

static void tick_handler(struct tm *tick_time, TimeUnits changed) {
 //char s_day_buffer[] = "ABC";
  
  // Store time
  s_last_time.hours = tick_time->tm_hour;
  s_last_time.hours -= (s_last_time.hours > 12) ? 12 : 0;
  s_last_time.minutes = tick_time->tm_min;

  /*for(int i = 0; i < 3; i++) {
  //  s_color_channels[i] = rand() % 256;
  }*/
 update_time();
  // Redraw
  if(s_canvas_layer) {
    layer_mark_dirty(s_canvas_layer);
  }
}

static int hours_to_minutes(int hours_out_of_12) {
  return (int)(float)(((float)hours_out_of_12 / 12.0F) * 60.0F);
}


static void update_proc(Layer *layer, GContext *ctx) {
 
  
  // Color background?
  GRect bounds = layer_get_bounds(layer);
  if(COLORS) {
    graphics_context_set_fill_color(ctx, GColorFromRGB(0, 0, 170));
    graphics_fill_rect(ctx, bounds, 0, GCornerNone);
  }

  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_context_set_stroke_width(ctx, 3);

  graphics_context_set_antialiased(ctx, ANTIALIASING);

  // White clockface GColorIcterine
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_circle(ctx, s_center, s_radius);

  // Draw outline
  graphics_draw_circle(ctx, s_center, s_radius);
  
  
  graphics_context_set_fill_color(ctx, GColorWhite);
   graphics_context_set_stroke_color(ctx, GColorDarkGray);
   //graphics_context_set_stroke_width(ctx, 3);
  
  
  // Draw three small circles
  graphics_draw_circle(ctx, GPoint(s_mini_cercle_center.x -40 ,s_mini_cercle_center.y), diamsmall);
  graphics_fill_circle(ctx, GPoint(s_mini_cercle_center.x -40 ,s_mini_cercle_center.y), diamsmall);
  //graphics_context_set_fill_color(ctx, GColorCyan);
  graphics_draw_circle(ctx, GPoint(s_mini_cercle_center.x +40 ,s_mini_cercle_center.y), diamsmall);
  graphics_fill_circle(ctx, GPoint(s_mini_cercle_center.x +40 ,s_mini_cercle_center.y), diamsmall);
  //graphics_context_set_fill_color(ctx, GColorCyan);
  graphics_draw_circle(ctx, GPoint(s_mini_cercle_center.x ,s_mini_cercle_center.y +40)  , (diamsmall + 17));
  graphics_fill_circle(ctx, GPoint(s_mini_cercle_center.x ,s_mini_cercle_center.y + 40)  , (diamsmall + 17));
 
 //draw days circle
  GRect roulette_space = GRect((s_mini_cercle_center.x - 35), (s_mini_cercle_center.y + 5 ) , DC_SIZE, DC_SIZE);
  graphics_context_set_compositing_mode(ctx, GCompOpSet);
  graphics_draw_bitmap_in_rect(ctx, s_example_bitmap, roulette_space); 
 

   // draw days roulette
  GRect start = GRect((s_mini_cercle_center.x - 35), (s_mini_cercle_center.y + 5 ) , DC_SIZE, DC_SIZE);
  graphics_context_set_fill_color(ctx, GColorElectricUltramarine);
  graphics_fill_radial(ctx, start, GOvalScaleModeFitCircle, 100,
                      DEG_TO_TRIGANGLE(day_border_1), DEG_TO_TRIGANGLE(day_border_2)); 
  
  //draw central roulette point
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_context_set_stroke_color(ctx, GColorPictonBlue);
  graphics_context_set_stroke_width(ctx,5);
  graphics_draw_circle(ctx, GPoint(s_mini_cercle_center.x ,s_mini_cercle_center.y +40) , 10);
  graphics_fill_circle(ctx, GPoint(s_mini_cercle_center.x ,s_mini_cercle_center.y +40) , 10); 
  
}
static void update_proc_hands(Layer *layer, GContext *ctx){
  // Don't use current time while animating
  Time mode_time = (s_animating) ? s_anim_time : s_last_time;

  // Adjust for minutes through the hour
  float minute_angle = TRIG_MAX_ANGLE * mode_time.minutes / 60;
  float hour_angle;
  if(s_animating) {
    // Hours out of 60 for smoothness
    hour_angle = TRIG_MAX_ANGLE * mode_time.hours / 60;
  } else {
    hour_angle = TRIG_MAX_ANGLE * mode_time.hours / 12;
  }
  hour_angle += (minute_angle / TRIG_MAX_ANGLE) * (TRIG_MAX_ANGLE / 12);

  // Plot hands
  GPoint minute_hand = (GPoint) {
    .x = (int16_t)(sin_lookup(TRIG_MAX_ANGLE * mode_time.minutes / 60) * (int32_t)(s_radius - HAND_MARGIN + 5) / TRIG_MAX_RATIO) + s_center.x,
    .y = (int16_t)(-cos_lookup(TRIG_MAX_ANGLE * mode_time.minutes / 60) * (int32_t)(s_radius - HAND_MARGIN + 5) / TRIG_MAX_RATIO) + s_center.y,
  };
  GPoint hour_hand = (GPoint) {
    .x = (int16_t)(sin_lookup(hour_angle) * (int32_t)(s_radius - (2 * HAND_MARGIN ) - 6) / TRIG_MAX_RATIO) + s_center.x,
    .y = (int16_t)(-cos_lookup(hour_angle) * (int32_t)(s_radius - (2 * HAND_MARGIN ) - 6 ) / TRIG_MAX_RATIO) + s_center.y,
  };

 
 // get second hour hand
  GRect hour_rect = GRect(s_center.x, s_center.y, -(s_center.x - hour_hand.x ), -(s_center.y - hour_hand.y));
  s_hour_center = grect_center_point(&hour_rect);
  
   // Draw hands with positive length only
  graphics_context_set_stroke_color(ctx, GColorOxfordBlue);
  
  if(s_radius > 2 * HAND_MARGIN) {
    graphics_context_set_stroke_width(ctx,6);
    graphics_draw_line(ctx, s_center, hour_hand);
    graphics_context_set_stroke_width(ctx,3);
    graphics_context_set_stroke_color(ctx, GColorCyan);
   graphics_draw_line(ctx, s_hour_center, hour_hand);
  } 
  if(s_radius > HAND_MARGIN) {
     graphics_context_set_stroke_color(ctx, GColorOxfordBlue);
     graphics_context_set_stroke_width(ctx,6);
    graphics_draw_line(ctx, s_center, minute_hand);
     graphics_context_set_stroke_width(ctx,3);
    graphics_context_set_stroke_color(ctx, GColorCyan);
    graphics_draw_line(ctx, GPoint((s_center.x), (s_center.y)), minute_hand);
  }
  
  // Draw central circle GColorCyan
  graphics_context_set_fill_color(ctx, GColorOxfordBlue);
  graphics_draw_circle(ctx, s_center, 3);
  graphics_context_set_stroke_color(ctx, GColorCyan);
  graphics_context_set_stroke_width(ctx,5);
  graphics_fill_circle(ctx, s_center, 3); 
  
 
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  static BitmapLayer *s_bitmap_layer;
//  Layer *s_days_arc= window_get_root_layer(window);
  GRect window_bounds = layer_get_bounds(window_layer);
  s_center = grect_center_point(&window_bounds);
  s_mini_cercle_center =  grect_center_point(&window_bounds);
 
 
  
  s_canvas_layer = layer_create(window_bounds);
  s_hands_layer = layer_create(window_bounds);
//  s_c_arc = layer_create(days_bounds);
  // draw bitmap GPoint(s_mini_cercle_center.x -40 ,s_mini_cercle_center.y)
  time_layer_h = text_layer_create(GRect((s_mini_cercle_center.x -53) , s_mini_cercle_center.y-13, 25, 22));
  time_layer_m = text_layer_create(GRect((s_mini_cercle_center.x +29) ,s_mini_cercle_center.y-13, 25, 22));
  
  text_layer_set_background_color(time_layer_h, GColorWhite);
  text_layer_set_text_color(time_layer_h, GColorElectricUltramarine);
  text_layer_set_text(time_layer_h, "00");
  text_layer_set_font(time_layer_h, fonts_get_system_font(FONT_KEY_ROBOTO_CONDENSED_21));
  text_layer_set_text_alignment(time_layer_h, GTextAlignmentCenter);
  text_layer_set_background_color(time_layer_m, GColorWhite);
  text_layer_set_text_color(time_layer_m, GColorElectricUltramarine);
  text_layer_set_text(time_layer_m, "00");
  text_layer_set_font(time_layer_m, fonts_get_system_font(FONT_KEY_ROBOTO_CONDENSED_21));
  text_layer_set_text_alignment(time_layer_m, GTextAlignmentCenter);
 //draw roulette
s_example_bitmap = gbitmap_create_with_resource(RESOURCE_ID_ROULETTE);
s_bitmap_layer = bitmap_layer_create(GRect((s_mini_cercle_center.x - 35), (s_mini_cercle_center.y + 5 ) , DC_SIZE, DC_SIZE));
//bitmap_layer_set_bitmap(s_bitmap_layer, s_example_bitmap);
 
 

   #if defined(PBL_BW)
  bitmap_layer_set_compositing_mode(s_bitmap_layer, GCompOpAssign);
#elif defined(PBL_COLOR)
  bitmap_layer_set_compositing_mode(s_bitmap_layer, GCompOpSet);

#endif

 // layer_add_child(window_layer, bitmap_layer_get_layer(s_bitmap_layer));
 
     layer_set_update_proc(s_canvas_layer, update_proc);
    layer_set_update_proc(s_hands_layer, update_proc_hands);
   layer_add_child(window_layer, s_canvas_layer);
 
   layer_add_child(window_get_root_layer(window), text_layer_get_layer(time_layer_h));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(time_layer_m));
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_bitmap_layer));
   layer_add_child(window_layer, s_hands_layer);
  
}

static void window_unload(Window *window) {
  layer_destroy(s_canvas_layer);
  gbitmap_destroy(s_example_bitmap);
  bitmap_layer_destroy(s_bitmap_layer);
}

/*********************************** App **************************************/

static int anim_percentage(AnimationProgress dist_normalized, int max) {
  return (int)(float)(((float)dist_normalized / (float)ANIMATION_NORMALIZED_MAX) * (float)max);
}

static void radius_update(Animation *anim, AnimationProgress dist_normalized) {
  s_radius = anim_percentage(dist_normalized, FINAL_RADIUS);

  layer_mark_dirty(s_canvas_layer);
}

static void hands_update(Animation *anim, AnimationProgress dist_normalized) {
  s_anim_time.hours = anim_percentage(dist_normalized, hours_to_minutes(s_last_time.hours));
  s_anim_time.minutes = anim_percentage(dist_normalized, s_last_time.minutes);

  layer_mark_dirty(s_canvas_layer);
}

static void init() {
   s_main_window = window_create();
  #ifdef PBL_SDK_2
  window_set_fullscreen(s_main_window, true);
#endif
  window_set_background_color(s_main_window, COLOR_FALLBACK(GColorBlue, GColorBlack));
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(s_main_window, true);
  srand(time(NULL));

  time_t t = time(NULL);
  struct tm *time_now = localtime(&t);
  tick_handler(time_now, MINUTE_UNIT);

  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);

  // Prepare animations
  AnimationImplementation radius_impl = {
    .update = radius_update
  };
  animate(ANIMATION_DURATION, ANIMATION_DELAY, &radius_impl, false);

  AnimationImplementation hands_impl = {
    .update = hands_update
  };
  animate(2 * ANIMATION_DURATION, ANIMATION_DELAY, &hands_impl, true);
}

static void deinit() {
  window_destroy(s_main_window);
}

int main() {
  init();
  app_event_loop();
  deinit();
}
