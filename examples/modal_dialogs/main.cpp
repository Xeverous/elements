/*=============================================================================
   Copyright (c) 2020 Micahl Urbanski

   Distributed under the MIT License (https://opensource.org/licenses/MIT)
=============================================================================*/
#include <elements.hpp>
#include <utility>

using namespace cycfi::elements;

auto make_widgets(view& view_, window& window_)
{
   auto text_box = share(static_text_box(""));

   auto button_save_file = button("save file");
   auto button_open_file = button("open files(s)");
   auto button_open_directory = button("open directory");

   button_save_file.on_click = [text_box, &window_, &view_](bool)
   {
      save_file_modal_settings settings;
      settings.initial_filename = "Untitled Document";
      auto path = window_.save_file_modal()({}, settings);
      text_box->set_text("saved file: " + path);
      view_.refresh(*text_box);
   };

   button_open_file.on_click = [text_box, &window_, &view_](bool)
   {
      open_file_modal_settings settings;
      settings.multiple_selection = true;
      auto paths = window_.open_file_modal()({}, settings);

      std::string result_text = "opened files(s):\n";
      for (const auto& path : paths)
         result_text.append(path).append("\n");

      text_box->set_text(result_text);
      view_.refresh(*text_box);
   };

   button_open_directory.on_click = [text_box, &window_, &view_](bool)
   {
      auto path = window_.open_directory_modal()({});
      text_box->set_text("opened directory: " + path);
      view_.refresh(*text_box);
   };

   return group("filesystem modal dialogs", margin(rect{ 10, 40, 10, 10 },
         vtile(
            htile(
               std::move(button_save_file),
               std::move(button_open_file),
               std::move(button_open_directory)
            ),
            hold(std::move(text_box))
         )
      ));
}

int main(int argc, char* argv[])
{
   app _app(argc, argv);
   window _win(_app.name());
   _win.on_close = [&_app]() { _app.stop(); };

   view _view(_win);

   _view.content(
      make_widgets(_view, _win),
      box(rgba(35, 35, 37, 255))
   );

   _app.run();
}
