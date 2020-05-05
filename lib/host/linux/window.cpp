/*=============================================================================
   Copyright (c) 2016-2020 Joel de Guzman

   Distributed under the MIT License (https://opensource.org/licenses/MIT)
=============================================================================*/
#include <elements/window.hpp>
#include <elements/support.hpp>
#include <functional>
#include <vector>
#include <memory>
#include <gtk/gtk.h>

namespace {
   using namespace cycfi::elements;

   struct gtk_widget_deleter
   {
      void operator()(GtkWidget* w) const
      {
         if (w != nullptr)
            gtk_widget_destroy(w);
      }
   };

   template <typename T>
   using gtk_widget_ptr = std::unique_ptr<T, gtk_widget_deleter>;

   struct glib_deleter
   {
      void operator()(gpointer p) const
      {
         g_free(p);
      }
   };

   template <typename T>
   using glib_ptr = std::unique_ptr<T, glib_deleter>;

   void apply_filesystem_modal_settings(GtkFileChooser& chooser, filesystem_modal_settings const& settings)
   {
      if (!settings.initial_directory.empty())
         gtk_file_chooser_set_current_folder(&chooser, settings.initial_directory.c_str());

      for (auto const& place : settings.additional_places)
      {
         if (!gtk_file_chooser_add_shortcut_folder(&chooser, place.c_str(), nullptr))
         {
            throw std::runtime_error("gtk_file_chooser_add_shortcut_folder() failed for " + place);
         }
      }

      gtk_file_chooser_set_local_only(&chooser, settings.allow_external_filesystem ? FALSE : TRUE);
      gtk_file_chooser_set_show_hidden(&chooser, settings.show_hidden_files ? TRUE : FALSE);
   }

   gtk_widget_ptr<GtkWidget> make_file_chooser_dialog(gchar const* title, GtkFileChooserAction action)
   {
      gtk_widget_ptr<GtkWidget> dialog(gtk_file_chooser_dialog_new(
         title,
         nullptr, // parent window
         action,
         "Cancel",
         GTK_RESPONSE_CANCEL,
         action == GTK_FILE_CHOOSER_ACTION_SAVE ? "Save" : "Open",
         GTK_RESPONSE_ACCEPT,
         nullptr
      ));

      if (!dialog)
         throw std::runtime_error("gtk_file_chooser_dialog_new() failed");

      return dialog;
   }

   std::string do_save_file_modal(
      filesystem_modal_settings const& fs_modal_st,
      save_file_modal_settings const& file_modal_st)
   {
      auto dialog = make_file_chooser_dialog(
         fs_modal_st.window_title.empty() ? "Save File" : fs_modal_st.window_title.c_str(),
         GTK_FILE_CHOOSER_ACTION_SAVE);

      GtkFileChooser& chooser = *GTK_FILE_CHOOSER(dialog.get());
      apply_filesystem_modal_settings(chooser, fs_modal_st);

      gtk_file_chooser_set_do_overwrite_confirmation(&chooser, file_modal_st.confirm_overwrite ? TRUE : FALSE);
      gtk_file_chooser_set_current_name(&chooser, file_modal_st.initial_filename.c_str());

      auto const res = gtk_dialog_run(GTK_DIALOG(dialog.get()));
      if (res != GTK_RESPONSE_ACCEPT)
         return {};

      glib_ptr<char> filename(gtk_file_chooser_get_filename(&chooser));
      return filename.get();
   }

   std::vector<std::string> do_open_file_modal(
      filesystem_modal_settings const& fs_modal_st,
      open_file_modal_settings const& file_modal_st)
   {
      auto dialog = make_file_chooser_dialog(
         fs_modal_st.window_title.empty() ? "Open File(s)" : fs_modal_st.window_title.c_str(),
         GTK_FILE_CHOOSER_ACTION_OPEN);

      GtkFileChooser& chooser = *GTK_FILE_CHOOSER(dialog.get());
      apply_filesystem_modal_settings(chooser, fs_modal_st);

      gtk_file_chooser_set_select_multiple(&chooser, file_modal_st.multiple_selection ? TRUE : FALSE);

      gint const res = gtk_dialog_run(GTK_DIALOG(dialog.get()));
      if (res != GTK_RESPONSE_ACCEPT)
         return {};

      struct list_deleter
      {
         void operator()(GSList* list) const
         {
            for (GSList* p = list; p != nullptr; p = p->next)
            {
               g_free(p->data);
            }

            g_slist_free(list);
         }
      };
      std::unique_ptr<GSList, list_deleter> list(gtk_file_chooser_get_filenames(&chooser));

      std::vector<std::string> result;
      for (GSList* p = list.get(); p != nullptr; p = p->next)
      {
         result.emplace_back(static_cast<char*>(p->data));
      }
      return result;
   }

   std::string do_open_directory_modal(
      filesystem_modal_settings const& fs_modal_st)
   {
      auto dialog = make_file_chooser_dialog(
         fs_modal_st.window_title.empty() ? "Open Directory" : fs_modal_st.window_title.c_str(),
         GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);

      GtkFileChooser& chooser = *GTK_FILE_CHOOSER(dialog.get());
      apply_filesystem_modal_settings(chooser, fs_modal_st);

      auto const res = gtk_dialog_run(GTK_DIALOG(dialog.get()));
      if (res != GTK_RESPONSE_ACCEPT)
         return {};

      glib_ptr<char> filename(gtk_file_chooser_get_filename(&chooser));
      return filename.get();
   }
}

namespace elements = cycfi::elements;

namespace cycfi { namespace elements
{
   struct host_window
   {
      GtkWidget* host = nullptr;
      std::vector<std::function<void()>> on_activate;
   };

   extern std::vector<std::function<void()>> on_activate;
   GtkApplication* get_app();
   bool app_is_activated();

   GtkWidget* get_window(host_window& h)
   {
      return h.host;
   }

   void on_window_activate(host_window& h, std::function<void()> f)
   {
      h.on_activate.push_back(f);
   }

   float get_scale(GtkWidget* widget)
   {
      auto gdk_win = gtk_widget_get_window(widget);
      return 1.0f / gdk_window_get_scale_factor(gdk_win);
   }

   window::window(std::string const& name, int style_, rect const& bounds)
    :  _window(new host_window)
   {
      // Chicken and egg. GTK wants us to create windows only
      // after the app is activated. So we have a scheme to
      // defer actions. If the app is not activated make_window is
      // added to the on_activate vector, otherwise, it is called
      // immediately.

      auto make_window =
         [this, name, style_, bounds]()
         {
            GtkWidget* win = gtk_application_window_new(get_app());
            g_object_ref(win);
            gtk_window_set_title(GTK_WINDOW(win), name.c_str());
            _window->host = win;

            for (auto f : _window->on_activate)
               f();

            gtk_widget_show_all(win);

            position(bounds.top_left());
            size(bounds.bottom_right());
         };

      if (app_is_activated())
         make_window();
      else
         on_activate.push_back(make_window);
   }

   window::~window()
   {
      g_object_unref(_window->host);
      delete _window;
   }

   point window::size() const
   {
      auto win = GTK_WINDOW(_window->host);
      auto scale = get_scale(_window->host);
      gint width, height;
      gtk_window_get_size(win, &width, &height);
      return { float(width) / scale, float(height) / scale };
   }

   void window::size(point const& p)
   {
      auto win = GTK_WINDOW(_window->host);
      auto scale = get_scale(_window->host);
      gtk_window_resize(win, p.x * scale, p.y * scale);
   }

   void window::limits(view_limits limits_)
   {
      auto set_limits =
         [this, limits_]()
         {
            constexpr float max = 10E6;
            auto win = GTK_WINDOW(_window->host);
            GdkGeometry hints;
            hints.min_width = limits_.min.x;
            hints.max_width = std::min<float>(limits_.max.x, max);
            hints.min_height = limits_.min.y;
            hints.max_height = std::min<float>(limits_.max.y, max);

            gtk_window_set_geometry_hints(
               win,
               _window->host,
               &hints,
               GdkWindowHints(GDK_HINT_MIN_SIZE | GDK_HINT_MAX_SIZE)
            );
         };

      if (app_is_activated())
         set_limits();
      else
         on_activate.push_back(set_limits);
   }

   point window::position() const
   {
      return {}; // $$$ TODO $$$
   }

   void window::position(point const& p)
   {
      auto set_position =
         [this, p]()
         {
            auto win = GTK_WINDOW(_window->host);
            auto scale = get_scale(_window->host);
            gtk_window_move(win, p.x * scale, p.y * scale);
         };

      if (app_is_activated())
         set_position();
      else
         on_activate.push_back(set_position);
   }

   save_file_fn window::save_file_modal()
   {
      return do_save_file_modal;
   }

   open_file_fn window::open_file_modal()
   {
      return do_open_file_modal;
   }

   open_directory_fn window::open_directory_modal()
   {
      return do_open_directory_modal;
   }

}}

