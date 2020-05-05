/*=============================================================================
   Copyright (c) 2020 Michal Urbanski

   Distributed under the MIT License (https://opensource.org/licenses/MIT)
=============================================================================*/

#include "utils.hpp"

#include <system_error>

namespace cycfi::elements {

   std::string utf16_to_utf8(std::wstring_view wstr)
   {
      if (wstr.empty())
         return {};
      int size = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), static_cast<int>(wstr.size()), nullptr, 0, nullptr, nullptr);
      std::string result(size, '\0');
      WideCharToMultiByte(CP_UTF8, 0, wstr.data(), static_cast<int>(wstr.size()), result.data(), size, nullptr, nullptr);
      return result;
   }

   std::wstring utf8_to_utf16(std::string_view str)
   {
      if (str.empty())
         return {};
      int size = MultiByteToWideChar(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), nullptr, 0);
      std::wstring result(size, L'\0');
      MultiByteToWideChar(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), result.data(), size);
      return result;
   }

   void throw_windows_error(HRESULT hr, char const* what_arg)
   {
      throw std::system_error(hr, std::system_category(), what_arg);
   }

}
