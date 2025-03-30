#include <Arduino.h>
#include <ESP_Panel_Library.h>
#include <lvgl.h>
#include "lvgl_port_v8.h"
#include <ESP_Knob.h>
#include <Button.h>
#include <ui.h>

// 新增全局变量
#define MESSAGE_GEAR_SHIFT "KNOB_GEAR_SHIFT::"
#define MESSAGE_KNOB_VALUE "KNOB_VALUE::"
#define MESSAGE_KNOB_ROTATE "KNOB_ROTATE::"
#define MESSAGE_KNOB_PUSH "KNOB_PRESSED::"
#define MESSAGE_KNOB_PUSH_DOUBLE "KNOB_DOUBLE_PRESSED::"
#define MESSAGE_KNOB_LONG "KNOB_LONG_PRESSED::"

#ifdef KNOB21
#define GPIO_NUM_KNOB_PIN_A 5
#define GPIO_NUM_KNOB_PIN_B 6
#define GPIO_BUTTON_PIN GPIO_NUM_0
#endif

int encoder = 0;
int knob_value = 0;
const char *GEARS = "PRND";
char last_gear[2] = "P";

enum knob_mode
{
  KNOB_GEAR_SHIFT,
  KNOB_ROTATE,
};

uint8_t knob_mode_flag = knob_mode::KNOB_GEAR_SHIFT;

// LVGL 组件定义
lv_obj_t *main_label;
// lv_obj_t *label_encoder;

ESP_Knob *knob;
Button *btn;

// 更新 LVGL 显示内容
void update_display() {

  switch(knob_mode_flag){
    case KNOB_GEAR_SHIFT:
      update_gear_string();
      break;
    case KNOB_ROTATE:
      update_knob_value();
      break;
  }

}

void update_knob_value(){
  // 更新编码器值标签
  char value_str[4];
  snprintf(value_str, 4, "%d", knob_value);
  lv_obj_set_style_text_color(main_label, lv_color_hex(0x6A3D9A), LV_PART_MAIN);
  lv_label_set_text(main_label, value_str);
  lv_obj_align(main_label, LV_ALIGN_CENTER, -25 * abs((int)strlen(value_str)), -60);

  Serial.printf("%s%d\n", MESSAGE_KNOB_VALUE, knob_value);
}

void update_gear_string() {
  // 计算档位
  knob_value = min(max(knob_value, 0), 3);
  char gear = GEARS[knob_value];
  char gearStr[2] = { gear, '\0' };

  // 串口输出
  if (strcmp(last_gear, gearStr)) {
    strcpy(last_gear, gearStr);
    Serial.printf("%s%s\n", MESSAGE_GEAR_SHIFT, last_gear);
  }

  // 更新档位标签
  lv_obj_set_style_text_color(main_label, lv_color_hex(0x4E3488), LV_PART_MAIN);
  lv_label_set_text(main_label, gearStr);
  lv_obj_align(main_label, LV_ALIGN_CENTER, -30, -60);
}

// 旋钮事件回调
void onKnobLeftEventCallback(int count, void *usr_data) {
  encoder = min(0, encoder);
  encoder--;
  if (encoder % 5 != 0){
    return;
  }
  knob_value--;
  if (knob_mode_flag == knob_mode::KNOB_ROTATE){
    Serial.printf("%s-\n", MESSAGE_KNOB_ROTATE);
  }

  lvgl_port_lock(-1);
  update_display();
  lvgl_port_unlock();
}

void onKnobRightEventCallback(int count, void *usr_data) {
  encoder = max(0, encoder);
  encoder++;
  
  if (encoder % 5 != 0){
    return;
  }

  knob_value++;
  if (knob_mode_flag == knob_mode::KNOB_ROTATE){
    Serial.printf("%s+\n", MESSAGE_KNOB_ROTATE);
  }
  

  lvgl_port_lock(-1);
  update_display();
  lvgl_port_unlock();
}

// 按钮事件回调
static void SingleClickCb(void *button_handle, void *usr_data) {

  Serial.printf("%s%d\n",MESSAGE_KNOB_PUSH, knob_mode_flag);

   switch(knob_mode_flag)
  {
    case knob_mode::KNOB_GEAR_SHIFT:
      break;
    case knob_mode::KNOB_ROTATE:
      knob_value = 0;
      break;
  }

  lvgl_port_lock(-1);
  update_display();
  lvgl_port_unlock();
}

static void DoubleClickCb(void *button_handle, void *usr_data) {
  knob_value = 0;
  switch(knob_mode_flag)
  {
    case knob_mode::KNOB_GEAR_SHIFT:
      knob_mode_flag = knob_mode::KNOB_ROTATE;
      break;
    case knob_mode::KNOB_ROTATE:
      knob_mode_flag = knob_mode::KNOB_GEAR_SHIFT;
      break;
  }

  Serial.printf("%s%d\n",MESSAGE_KNOB_PUSH_DOUBLE, knob_mode_flag);
  lvgl_port_lock(-1);
  update_display();
  lvgl_port_unlock();
}


static void LongPressStartCb(void *button_handle, void *usr_data) {
  Serial.printf("%s%d\n",MESSAGE_KNOB_LONG, knob_mode_flag);
  lvgl_port_lock(-1);
  // 此处可添加按钮长按的 UI 操作
  lvgl_port_unlock();
}

void setup() {
  Serial.begin(115200);
  Serial.println("LVGL Gear Shift Demo");

  // 初始化硬件
  ESP_Panel *panel = new ESP_Panel();
  panel->init();
  panel->begin();

  // 初始化旋钮
  knob = new ESP_Knob(GPIO_NUM_KNOB_PIN_A, GPIO_NUM_KNOB_PIN_B);
  knob->begin();
  knob->attachLeftEventCallback(onKnobLeftEventCallback);
  knob->attachRightEventCallback(onKnobRightEventCallback);

  // 初始化按钮
  btn = new Button(GPIO_BUTTON_PIN, false);
  btn->attachSingleClickEventCb(&SingleClickCb, NULL);
  btn->attachDoubleClickEventCb(&DoubleClickCb, NULL);
  btn->attachLongPressStartEventCb(&LongPressStartCb, NULL);

  // 初始化 LVGL
  lvgl_port_init(panel->getLcd(), panel->getTouch());

  // 创建 UI 组件
  lvgl_port_lock(-1);
  lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x1F1F1F), LV_PART_MAIN);

  // 档位标签（大字体居中）
  lv_obj_t *parent = lv_scr_act();
  main_label = lv_label_create(parent);
  lv_obj_set_style_text_font(main_label, &lv_font_montserrat_40, LV_PART_MAIN);
  lv_label_set_text(main_label, "P");
  lv_obj_set_style_text_color(main_label, lv_color_hex(0x4E3488), LV_PART_MAIN);
  lv_obj_set_style_transform_zoom(main_label, 1024, LV_PART_MAIN);
  lv_obj_align(main_label, LV_ALIGN_CENTER, -25, -60);
  // lv_obj_set_align(main_label, LV_ALIGN_CENTER);
  // lv_obj_center(main_label);


  // 编码器值标签（底部显示）
  // label_encoder = lv_label_create(lv_scr_act());
  // lv_obj_set_style_text_font(label_encoder, &lv_font_montserrat_20, LV_PART_MAIN);
  // lv_label_set_text(label_encoder, "0");
  // lv_obj_align(label_encoder, LV_ALIGN_BOTTOM_MID, 0, -20);

  lvgl_port_unlock();
}

void loop() {
  delay(20);
}
