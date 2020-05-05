/*=============================================================================
   Copyright (c) 2020 Michal Urbanski

   Distributed under the MIT License (https://opensource.org/licenses/MIT)
=============================================================================*/
#if !defined(ELEMENTS_WINDOWS_UTILS_MAY_07_2020)
#define ELEMENTS_WINDOWS_UTILS_MAY_07_2020

#include <string>
#include <string_view>
#include <memory>

#include <Windows.h>
#include <objbase.h>

namespace cycfi::elements {

   std::string utf16_to_utf8(std::wstring_view wstr);
   std::wstring utf8_to_utf16(std::string_view str);

   inline float get_scale_for_window(HWND hwnd)
   {
      #ifdef ELEMENTS_HOST_ONLY_WIN7
      (void) hwnd;
      return 1.0f;
      #else
      return GetDpiForWindow(hwnd) / 96.0f;
      #endif
   }

   [[noreturn]]
   void throw_windows_error(HRESULT hr, char const* what_arg);

   struct iunknown_deleter
   {
      void operator()(IUnknown* p) const
      {
         if (p != nullptr)
            p->Release();
      }
   };

   template <typename T>
   using com_interface_ptr = std::unique_ptr<T, iunknown_deleter>;

   struct co_task_deleter
   {
      void operator()(LPVOID pv) const
      {
         CoTaskMemFree(pv);
      }
   };

   template <typename T>
   using co_task_ptr = std::unique_ptr<T, co_task_deleter>;

   template <typename T>
   com_interface_ptr<T> make_instance(REFCLSID rclsid)
   {
      T* p = nullptr;
      // https://docs.microsoft.com/en-us/windows/win32/api/combaseapi/nf-combaseapi-cocreateinstance
      // https://docs.microsoft.com/en-us/windows/win32/api/combaseapi/nf-combaseapi-iid_ppv_args
      HRESULT hr = CoCreateInstance(rclsid, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&p));

      if (!SUCCEEDED(hr))
         throw_windows_error(hr, "CoCreateInstance");

      return com_interface_ptr<T>(p);
   }

}

#endif
