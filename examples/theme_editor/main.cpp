/*=============================================================================
   Copyright (c) 2020 Michal Urbanski

   Distributed under the MIT License (https://opensource.org/licenses/MIT)
=============================================================================*/
#include <elements.hpp>
#include <string_view>
#include <algorithm>
#include <cstdlib>

using namespace cycfi::elements;

constexpr bool is_digit(char c)
{
   return '0' <= c && c <= '9';
}

auto make_input_box(
   std::string placeholder,
   std::string_view initial_text,
   basic_input_box::text_function callback)
{
   basic_input_box box{
      std::move(placeholder),
      get_theme().text_box_font,
      get_theme().text_box_font_size
   };

   box.set_text(initial_text);
   box.on_text = std::move(callback);
   return input_box(std::move(box));
}

auto make_color_float_input_box(std::string placeholder, float& fl)
{
   auto color_float_input_callback = [&fl](std::string_view str)
   {
      std::string result;

      for (char c : str)
      {
         if (is_digit(c))
            result.append(1, c);
      }

      if (result.empty())
      {
         fl = 0;
      }
      else
      {
         int val = std::strtol(result.c_str(), nullptr, 10);
         val = std::clamp(val, 0, 255);
         fl = val / 255.0f;
      }

      return result;
   };

   return make_input_box(
      std::move(placeholder),
      std::to_string(static_cast<int>(fl * 255.0f)),
      color_float_input_callback);
}

auto make_color_input(std::string name, color& c)
{
   return htile(
      label(std::move(name)),
      make_color_float_input_box("r", c.red),
      make_color_float_input_box("g", c.green),
      make_color_float_input_box("b", c.blue),
      make_color_float_input_box("a", c.alpha)
   );
}

auto make_float_input_box(std::string name, float& fl)
{
   auto float_input_callback = [&fl](std::string_view str)
   {
      std::string result(str);
      fl = std::strtof(result.c_str(), nullptr);
      fl = std::clamp(fl, 0.0f, 100.0f);

      return result;
   };

   return make_input_box(std::move(name), std::to_string(fl), float_input_callback);
}

auto make_float_input(std::string name, float& fl)
{
   return htile(
      label(std::move(name)),
      make_float_input_box("value", fl)
   );
}

auto make_rect_input(std::string name, rect& r)
{
   return htile(
      label(std::move(name)),
      make_float_input_box("left", r.left),
      make_float_input_box("top", r.top),
      make_float_input_box("right", r.right),
      make_float_input_box("bottom", r.bottom)
   );
}

auto make_extent_input(std::string name, extent& ex)
{
   return htile(
      label(std::move(name)),
      make_float_input_box("x", ex.x),
      make_float_input_box("y", ex.y)
   );
}

auto make_int_input(std::string name, int& n)
{
   auto int_input_callback = [&n](std::string_view str)
   {
      std::string result(str);
      n = std::strtol(result.c_str(), nullptr, 10);
      n = std::clamp(n, 0, 500);

      return result;
   };

   return htile(
      label(std::move(name)),
      make_input_box("value", std::to_string(n), int_input_callback)
   );
}

auto make_theme_input(theme& theme_)
{
   return vtile();
/*
   vscroller(vtile(
      make_color_input("panel color", theme_.panel_color),
      make_color_input("frame color", theme_.frame_color),
      make_float_input("frame corner radius", theme_.frame_corner_radius),
      make_float_input("frame stroke width", theme_.frame_stroke_width),
      make_color_input("scrollbar color", theme_.scrollbar_color),
      make_color_input("default button color", theme_.default_button_color),
      make_rect_input("button margin", theme_.button_margin),

      make_color_input("controls color", theme_.controls_color),
      make_color_input("indicator color", theme_.indicator_color),
      make_color_input("basic font color", theme_.basic_font_color),
      // system font (skipped)

      make_float_input("box widget background opacity", theme_.box_widget_bg_opacity),

      make_color_input("heading font color", theme_.heading_font_color),
      // heading font (skipped)
      make_float_input("heading font size", theme_.heading_font_size),
      make_int_input("heading text align", theme_.heading_text_align),

      make_color_input("label font color", theme_.label_font_color),
      // label font (skipped)
      make_float_input("label font size", theme_.label_font_size),
      make_int_input("label text align", theme_.label_text_align),

      make_color_input("icon_color", theme_.icon_color),
      // icon font (skipped)
      make_float_input("icon font size", theme_.icon_font_size),
      make_color_input("icon button color", theme_.icon_button_color),

      make_color_input("text box font color", theme_.text_box_font_color),
      // text box font (skipped)
      make_float_input("text box font size", theme_.text_box_font_size),
      make_color_input("text box hilite color", theme_.text_box_hilite_color),
      make_color_input("text box caret color", theme_.text_box_caret_color),
      make_float_input("text box caret width", theme_.text_box_caret_width),
      make_color_input("inactive font color", theme_.inactive_font_color),

      make_color_input("ticks color", theme_.ticks_color),
      make_float_input("major ticks level", theme_.major_ticks_level),
      make_float_input("major ticks width", theme_.major_ticks_width),
      make_float_input("minor ticks level", theme_.minor_ticks_level),
      make_float_input("minor ticks width", theme_.minor_ticks_width),

      make_color_input("major grid color", theme_.major_grid_color),
      make_float_input("major grid width", theme_.major_grid_width),
      make_color_input("minor grid color", theme_.minor_grid_color),
      make_float_input("minor grid width", theme_.minor_grid_width),

      make_float_input("dialog button size", theme_.dialog_button_size),
      make_extent_input("message textbox size", theme_.message_textbox_size)
   ));
   */
}

auto make_examples(theme& theme_)
{
   return layer(
      margin({20, 20, 20, 20}, vtile(
         progress_bar(rbox(colors::black), rbox(theme_.indicator_color), 0.5),
         button(icons::cog, "test", 1.0, theme_.default_button_color),
         layer(margin({20, 20, 20, 20}, align_center_middle(label("label on a panel"))), panel{1.0f})
      )),
      panel{}
   );
}

void setup_interface(view& view_, theme& theme_);

auto make_interface(view& view_, theme& theme_)
{
   auto but = button("reload");
   but.on_click = [&view_, &theme_](bool)
   {
      setup_interface(view_, theme_);
   };

   return htile(
      vtile(
         make_theme_input(theme_),
         std::move(but)
      ),
      make_examples(theme_)
   );
}

void setup_interface(view& view_, theme& theme_)
{
   view_.content(
      make_interface(view_, theme_),
      box(rgba(35, 35, 37, 255))
   );
   view_.refresh();
}

int main(int argc, char* argv[])
{
   app _app(argc, argv);
   window _win(_app.name());
   _win.on_close = [&_app]() { _app.stop(); };

   view view_(_win);

   theme theme_ = get_theme();
   setup_interface(view_, theme_);

   _app.run();
   return 0;
}
