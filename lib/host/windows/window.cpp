/*=============================================================================
   Copyright (c) 2016-2020 Joel de Guzman

   Distributed under the MIT License (https://opensource.org/licenses/MIT)
=============================================================================*/
#include <elements/window.hpp>
#include <elements/support.hpp>

#include <windows.h>      // For common windows data types and function headers
#ifndef STRICT_TYPED_ITEMIDS
#define STRICT_TYPED_ITEMIDS
#endif
#include <shlobj.h>
#include <objbase.h>      // For COM headers
#include <shobjidl.h>     // for IFileDialogEvents and IFileDialogControlEvents
#include <shlwapi.h>
#include <knownfolders.h> // for KnownFolder APIs/datatypes/function headers
#include <propvarutil.h>  // for PROPVAR-related functions
#include <propkey.h>      // for the Property key APIs/datatypes
#include <propidl.h>      // for the Property System APIs
#include <strsafe.h>      // for StringCchPrintfW
#include <shtypes.h>      // for COMDLG_FILTERSPEC
#include <memory>

#include "utils.hpp"

namespace elements = cycfi::elements;

namespace cycfi { namespace elements
{
   namespace
   {
      struct window_info
      {
         window*     wptr = nullptr;
         view_limits limits = {};
      };

      window_info* get_window_info(HWND hwnd)
      {
         auto param = GetWindowLongPtrW(hwnd, GWLP_USERDATA);
         return reinterpret_cast<window_info*>(param);
      }

      void disable_close(HWND hwnd)
      {
         EnableMenuItem(GetSystemMenu(hwnd, FALSE), SC_CLOSE,
            MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
      }

      void disable_minimize(HWND hwnd)
      {
         SetWindowLongW(hwnd, GWL_STYLE,
            GetWindowLongW(hwnd, GWL_STYLE) & ~WS_MINIMIZEBOX);
      }

      void disable_maximize(HWND hwnd)
      {
         SetWindowLongW(hwnd, GWL_STYLE,
            GetWindowLongW(hwnd, GWL_STYLE) & ~WS_MAXIMIZEBOX);
      }

      void disable_resize(HWND hwnd)
      {
         SetWindowLongW(hwnd, GWL_STYLE,
            GetWindowLongW(hwnd, GWL_STYLE) & ~WS_SIZEBOX);
         disable_maximize(hwnd);
      }

      LRESULT on_close(window* win)
      {
         if (win)
            win->on_close();
         return 0;
      }

      BOOL CALLBACK for_each_child(HWND child, LPARAM lParam)
      {
         LPRECT bounds = (LPRECT) lParam;
         MoveWindow(
            child,
            0, 0,
            bounds->right,
            bounds->bottom,
            TRUE);

         // Make sure the child window is visible.
         ShowWindow(child, SW_SHOW);
         return true;
      }

      LRESULT on_size(HWND hwnd)
      {
         RECT bounds;
         GetClientRect(hwnd, &bounds);
         EnumChildWindows(hwnd, for_each_child, (LPARAM) &bounds);
         return 0;
      }

      POINT window_frame_size(HWND hwnd)
      {
         RECT content, frame;
         POINT extra;
         GetClientRect(hwnd, &content);
         GetWindowRect(hwnd, &frame);
         extra.x = (frame.right - frame.left) - content.right;
         extra.y = (frame.bottom - frame.top) - content.bottom;
         return extra;
      }

      void constrain_size(HWND hwnd, RECT& r, view_limits limits)
      {
         auto const scale = get_scale_for_window(hwnd);
         auto extra = window_frame_size(hwnd);
         auto w = ((r.right - r.left) - extra.x) / scale;
         auto h = ((r.bottom - r.top) - extra.y) / scale;

         if (w > limits.max.x)
            r.right = r.left + extra.x + (limits.max.x * scale);
         if (w < limits.min.x)
            r.right = r.left + extra.x + (limits.min.x * scale);
         if (h > limits.max.y)
            r.bottom = r.top + extra.y + (limits.max.y * scale);
         if (h < limits.min.y)
            r.bottom = r.top + extra.y + (limits.min.y * scale);
      }

      LRESULT CALLBACK handle_event(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
      {
         auto* info = get_window_info(hwnd);
         switch (message)
         {
            case WM_CLOSE: return on_close(info->wptr);

            case WM_DPICHANGED:
            case WM_SIZE: return on_size(hwnd);

            case WM_SIZING:
               if (info)
               {
                  auto& r = *reinterpret_cast<RECT*>(lparam);
                  constrain_size(hwnd, r, info->limits);
               }
               break;

            default:
               return DefWindowProcW(hwnd, message, wparam, lparam);
         }
         return 0;
      }

      struct init_window_class
      {
         init_window_class()
         {
            WNDCLASSW windowClass = {};
            windowClass.hbrBackground = nullptr;
            windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
            windowClass.hInstance = nullptr;
            windowClass.lpfnWndProc = handle_event;
            windowClass.lpszClassName = L"ElementsWindow";
            windowClass.style = CS_HREDRAW | CS_VREDRAW;
            if (!RegisterClassW(&windowClass))
               MessageBoxW(nullptr, L"Could not register class", L"Error", MB_OK);
         }
      };

      co_task_ptr<WCHAR> get_item_path(IShellItem& item)
      {
         // https://docs.microsoft.com/en-us/windows/win32/api/shobjidl_core/nf-shobjidl_core-ishellitem-getdisplayname
         LPWSTR path;
         HRESULT hr = item.GetDisplayName(SIGDN_FILESYSPATH, &path);

         if (!SUCCEEDED(hr))
            throw_windows_error(hr, "IShellItem::GetDisplayName");

         return co_task_ptr<WCHAR>(path);
      }

      com_interface_ptr<IShellItem> shell_item_from_path(std::wstring const& path)
      {
         void* item;
         HRESULT hr = SHCreateItemFromParsingName(path.c_str(), nullptr, IID_IShellItem, &item);

         if (!SUCCEEDED(hr))
            throw_windows_error(hr, "SHCreateItemFromParsingName");

         return com_interface_ptr<IShellItem>(static_cast<IShellItem*>(item));
      }

      com_interface_ptr<IShellItem> get_item_at(IShellItemArray& array, DWORD index)
      {
         IShellItem* item;
         HRESULT hr = array.GetItemAt(index, &item);

         if (!SUCCEEDED(hr))
            throw_windows_error(hr, "IShellItemArray::GetItemAt");

         return com_interface_ptr<IShellItem>(item);
      }

      com_interface_ptr<IShellItem> get_dialog_result(IFileDialog& dialog)
      {
         IShellItem* shell_item;
         HRESULT hr = dialog.GetResult(&shell_item);

         if (!SUCCEEDED(hr))
            throw_windows_error(hr, "IFileDialog::GetResult");

         return com_interface_ptr<IShellItem>(shell_item);
      }

      void apply_dialog_options(
         IFileDialog& dialog,
         filesystem_modal_settings const& fs_modal_st,
         bool confirm_overwrite,
         bool multiple_selection,
         bool follow_symlinks,
         bool path_must_exist,
         bool pick_folders)
      {
         // Before setting, always get the options first in order not to override default options.
         FILEOPENDIALOGOPTIONS options;
         HRESULT hr = dialog.GetOptions(&options);
         if (!SUCCEEDED(hr))
            throw_windows_error(hr, "IFileDialog::GetOptions");

         if (fs_modal_st.allow_external_filesystem)
            options &= ~FOS_FORCEFILESYSTEM;
         else
            options |= FOS_FORCEFILESYSTEM;

         if (fs_modal_st.file_preview)
            options |= FOS_FORCEPREVIEWPANEON;
         else
            options &= ~FOS_FORCEPREVIEWPANEON;

         if (fs_modal_st.show_hidden_files)
            options |= FOS_FORCESHOWHIDDEN;
         else
            options &= ~FOS_FORCESHOWHIDDEN;

         if (confirm_overwrite)
            options |= FOS_OVERWRITEPROMPT;
         else
            options &= ~FOS_OVERWRITEPROMPT;

         if (multiple_selection)
            options |= FOS_ALLOWMULTISELECT;
         else
            options &= ~FOS_ALLOWMULTISELECT;

         if (follow_symlinks)
            options &= ~FOS_NODEREFERENCELINKS;
         else
            options |= FOS_NODEREFERENCELINKS;

         if (path_must_exist)
            options |= FOS_FILEMUSTEXIST;
         else
            options &= ~FOS_FILEMUSTEXIST;

         if (pick_folders)
            options |= FOS_PICKFOLDERS;
         else
            options &= ~FOS_PICKFOLDERS;

         hr = dialog.SetOptions(options);
         if (!SUCCEEDED(hr))
            throw_windows_error(hr, "IFileDialog::SetOptions");

         if (!fs_modal_st.window_title.empty())
         {
            hr = dialog.SetTitle(utf8_to_utf16(fs_modal_st.window_title).c_str());
            if (!SUCCEEDED(hr))
               throw_windows_error(hr, "IFileDialog::SetTitle");
         }

         if (!fs_modal_st.initial_directory.empty())
         {
            auto item = shell_item_from_path(utf8_to_utf16(fs_modal_st.initial_directory));
            hr = dialog.SetFolder(item.get());
            if (!SUCCEEDED(hr))
               throw_windows_error(hr, "IFileDialog::SetFolder");
         }

         for (auto const& path : fs_modal_st.additional_places)
         {
            auto item = shell_item_from_path(utf8_to_utf16(path));
            hr = dialog.AddPlace(item.get(), FDAP_BOTTOM);
            if (!SUCCEEDED(hr))
               throw_windows_error(hr, "IFileDialog::AddPlace");
         }
      }

      /*
       * Modal dialog code based on:
       * https://github.com/microsoft/Windows-classic-samples/tree/master/Samples/Win7Samples/winui/shell/appplatform/commonfiledialog
       * We dropped the CDialogEventHandler code because for simplicity elements
       * does not support reacting to the events while the modal is open.
       */
      std::string do_save_file_modal(
         filesystem_modal_settings const& fs_modal_st,
         save_file_modal_settings const& file_modal_st)
      {
         // CoCreate the File Save Dialog object.
         auto dialog = make_instance<IFileSaveDialog>(CLSID_FileSaveDialog);
         apply_dialog_options(*dialog, fs_modal_st, file_modal_st.confirm_overwrite, false, true, true, false);

         if (!file_modal_st.initial_filename.empty())
         {
            HRESULT hr = dialog->SetFileName(utf8_to_utf16(file_modal_st.initial_filename).c_str());
            if (!SUCCEEDED(hr))
               throw_windows_error(hr, "IFileSaveDialog::SetFileName");
         }

         // Show the dialog
         // if the argument to show is a null pointer, the dialog is modeless
         HRESULT hr = dialog->Show(nullptr);
         if (!SUCCEEDED(hr))
         {
            // user cancelled the operation or something else aborted the dialog
            return {};
         }

         return utf16_to_utf8(get_item_path(*get_dialog_result(*dialog)).get());
      }
   }

   std::vector<std::string> do_open_file_modal(
      filesystem_modal_settings const& fs_modal_st,
      open_file_modal_settings const& file_modal_st)
   {
         // CoCreate the File Open Dialog object.
         auto dialog = make_instance<IFileOpenDialog>(CLSID_FileOpenDialog);
         apply_dialog_options(*dialog, fs_modal_st, false, file_modal_st.multiple_selection,
            file_modal_st.follow_symlinks, file_modal_st.path_must_exist, false);

         // Show the dialog
         // if the argument to show is a null pointer, the dialog is modeless
         HRESULT hr = dialog->Show(nullptr);
         if (!SUCCEEDED(hr))
         {
            // user cancelled the operation or something else aborted the dialog
            return {};
         }

         // Obtain the result, once the user clicks the button.
         // The result is an IShellItemArray object.
         // https://docs.microsoft.com/en-us/windows/win32/api/shobjidl_core/nf-shobjidl_core-ifileopendialog-getresults
         auto items = [&]()
         {
            IShellItemArray* shell_items;
            hr = dialog->GetResults(&shell_items);

            if (!SUCCEEDED(hr))
               throw_windows_error(hr, "IFileDialog::GetResults");

            return com_interface_ptr<IShellItemArray>(shell_items);
         }();

         DWORD num_items;
         hr = items->GetCount(&num_items);
         if (!SUCCEEDED(hr))
            throw_windows_error(hr, "IShellItemArray::GetCount");

         std::vector<std::string> results;
         for (DWORD i = 0; i < num_items; ++i)
         {
            auto item = get_item_at(*items, i);
            auto path = get_item_path(*item);
            results.push_back(utf16_to_utf8(path.get()));
         }

         return results;
   }

   std::string do_open_directory_modal(
      filesystem_modal_settings const& fs_modal_st)
   {
      auto dialog = make_instance<IFileOpenDialog>(CLSID_FileOpenDialog);
      apply_dialog_options(*dialog, fs_modal_st, false, false, true, true, true);

      // Show the dialog
      // if the argument to show is a null pointer, the dialog is modeless
      HRESULT hr = dialog->Show(nullptr);
      if (!SUCCEEDED(hr))
      {
         // user cancelled the operation or something else aborted the dialog
         return {};
      }

      return utf16_to_utf8(get_item_path(*get_dialog_result(*dialog)).get());
   }

   window::window(std::string const& name, int style_, rect const& bounds)
   {
      static init_window_class init;

      std::wstring wname = utf8_to_utf16(name);
      #ifdef ELEMENTS_HOST_ONLY_WIN7
      auto const scale = 1.0f;
      #else
      auto const scale = GetDpiForSystem() / 96.0f;
      #endif

      _window = CreateWindowW(
         L"ElementsWindow",
         wname.c_str(),
         WS_OVERLAPPEDWINDOW,
         bounds.left * scale, bounds.top * scale,
         bounds.width() * scale, bounds.height() * scale,
         nullptr, nullptr, nullptr,
         nullptr
      );

      auto* info = new window_info{ this };
      SetWindowLongPtrW(_window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(info));

      if (!(style_ & closable))
         disable_close(_window);
      if (!(style_ & miniaturizable))
         disable_minimize(_window);
      if (!(style_ & resizable))
         disable_resize(_window);

      ShowWindow(_window, SW_RESTORE);
   }

   window::~window()
   {
      delete get_window_info(_window);
      DeleteObject(_window);
   }

   point window::size() const
   {
      auto const scale = get_scale_for_window(_window);
      RECT frame;
      GetWindowRect(_window, &frame);
      return {
         float((frame.right - frame.left) / scale),
         float((frame.bottom - frame.top) / scale)
      };
   }

   void window::size(point const& p)
   {
      auto const scale = get_scale_for_window(_window);
      RECT frame;
      GetWindowRect(_window, &frame);
      frame.right = frame.left + (p.x * scale);
      frame.bottom = frame.top + (p.y * scale);
      constrain_size(
         _window, frame, get_window_info(_window)->limits);

      MoveWindow(
         _window, frame.left, frame.top,
         frame.right - frame.left,
         frame.bottom - frame.top,
         true // repaint
      );
   }

   void window::limits(view_limits limits_)
   {
      get_window_info(_window)->limits = limits_;
      RECT frame;
      GetWindowRect(_window, &frame);
      constrain_size(
         _window, frame, get_window_info(_window)->limits);

      MoveWindow(
         _window, frame.left, frame.top,
         frame.right - frame.left,
         frame.bottom - frame.top,
         true // repaint
      );
   }

   point window::position() const
   {
      auto const scale = get_scale_for_window(_window);
      RECT frame;
      GetWindowRect(_window, &frame);
      return { float(frame.left / scale), float(frame.top / scale) };
   }

   void window::position(point const& p)
   {
      auto const scale = get_scale_for_window(_window);
      RECT frame;
      GetWindowRect(_window, &frame);

      MoveWindow(
         _window, p.x * scale, p.y * scale,
         frame.right - frame.left,
         frame.bottom - frame.top,
         true // repaint
      );
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

