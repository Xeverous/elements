/*=============================================================================
   Copyright (c) 2016-2020 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include <elements/element/gallery/radio_button.hpp>

namespace cycfi { namespace elements
{

   void basic_radio_button::select(bool state)
   {
      if (state != is_selected())
      {
         selectable::select(state);
         value(state);
      }
   }

   element* basic_radio_button::click(context const& ctx, mouse_button btn)
   {
      auto r = basic_latching_button<>::click(ctx, btn);
      if (!is_selected())
      {
         auto [c, cctx] = find_composite(ctx);
         if (c)
         {
            for (std::size_t i = 0; i != c->size(); ++i)
            {
               if (auto e = find_subject<basic_radio_button>(&c->at(i)))
               {
                  if (e == this)
                     e->select(true);
                  else
                     e->select(false);
               }
            }
         }
         cctx->view.refresh(*cctx);
      }
      return r;
   }
}}
