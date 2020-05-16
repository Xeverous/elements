/*=============================================================================
   Copyright (c) 2016-2020 Joel de Guzman

   Distributed under the MIT License (https://opensource.org/licenses/MIT)
=============================================================================*/
#include <elements/app.hpp>
#include <json/json.hpp>
#include <json/json_io.hpp>
#include <infra/assert.hpp>
#include <infra/filesystem.hpp>
#include <windows.h>
#include <shlobj.h>
#include <cstring>

#ifndef ELEMENTS_HOST_ONLY_WIN7
#include <shellscalingapi.h>
#endif

#include "utils.hpp"

namespace cycfi { namespace elements
{
   struct config
   {
      std::string application_title;
      std::string application_copyright;
      std::string application_id;
      std::string application_version;
   };

}}


BOOST_FUSION_ADAPT_STRUCT(
   cycfi::elements::config,
   (std::string, application_title)
   (std::string, application_copyright)
   (std::string, application_id)
   (std::string, application_version)
)

namespace cycfi { namespace elements
{
   config get_config()
   {
      fs::path path = "config.json";

	  std::string fp = fs::absolute(path).string();

      CYCFI_ASSERT(fs::exists(path), "Error: config.json not exist.");
      auto r = json::load<config>(path);
      CYCFI_ASSERT(r, "Error: Invalid config.json.");
      return *r;
   }

   config app_config;

   struct init_app
   {
      init_app()
      {
         app_config = get_config();
      }
   };

   app::app(int /* argc */, char** /* argv */)
   {
      init_app init;
      _app_name = app_config.application_title;

      #ifndef ELEMENTS_HOST_ONLY_WIN7
      SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
      #endif
   }

   app::~app()
   {
   }

   void app::run()
   {
      MSG messages;
      while (_running && GetMessage(&messages, nullptr, 0, 0) > 0)
      {
         TranslateMessage(&messages);
         DispatchMessage(&messages);
      }
   }

   void app::stop()
   {
      _running = false;
   }

   fs::path app_data_path()
   {
      auto path = []()
      {
         PWSTR path = nullptr;
         SHGetKnownFolderPath(FOLDERID_ProgramData, KF_FLAG_CREATE, nullptr, &path);
         return co_task_ptr<WCHAR>(path);
      }();
      return fs::path{ path.get() };
   }
}}

