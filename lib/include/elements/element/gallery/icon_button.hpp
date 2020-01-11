/*=============================================================================
   Copyright (c) 2016-2019 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#if !defined(ELEMENTS_GALLERY_ICON_BUTTON_JUNE_5_2016)
#define ELEMENTS_GALLERY_ICON_BUTTON_JUNE_5_2016

#include <elements/element/element.hpp>
#include <elements/element/gallery/button.hpp>
#include <elements/support/theme.hpp>

namespace cycfi { namespace elements
{
   ////////////////////////////////////////////////////////////////////////////
   // Icon Button
   ////////////////////////////////////////////////////////////////////////////
   void draw_icon_button(
      context const& ctx
    , uint32_t code
    , float size
    , color body_color
    , bool state
    , bool hilite
   );

   template <bool state, bool hilite = state>
   struct icon_button_element : element
   {
                              icon_button_element(
                                 uint32_t code
                               , float size
                               , color body_color = get_theme().default_button_color
                              )
                               : _code(code)
                               , _size(size)
                               , _body_color(body_color)
                              {}

      virtual view_limits     limits(basic_context const& ctx) const;
      virtual void            draw(context const& ctx);

      uint32_t                _code;
      float                   _size;
      color                   _body_color;
   };

   template <bool state, bool hilite>
   inline view_limits icon_button_element<state, hilite>::limits(basic_context const& /* ctx */) const
   {
      auto  size = _size * get_theme().icon_font_size * 1.8f;
      return { { size, size }, { size, size } };
   }

   template <bool state, bool hilite>
   inline void icon_button_element<state, hilite>::draw(context const& ctx)
   {
      draw_icon_button(
         ctx, _code, _size * get_theme().icon_font_size, _body_color, state, hilite
      );
   }

   inline basic_toggle_button<> toggle_icon_button(
      uint32_t code
    , float size
    , color body_color = get_theme().default_button_color
   )
   {
      return {
         icon_button_element<false>{ code, size, body_color }
       , icon_button_element<true>{ code, size, body_color }
      };
   }

   inline basic_toggle_button<> toggle_icon_button(
      uint32_t code1
    , uint32_t code2
    , float size
    , color body_color = get_theme().default_button_color
   )
   {
      return {
         icon_button_element<true, true>{ code1, size, body_color }
       , icon_button_element<true, true>{ code2, size, body_color }
      };
   }

   inline layered_button icon_button(
      uint32_t code
    , float size
    , color body_color = get_theme().default_button_color
   )
   {
      return {
         icon_button_element<false>{ code, size, body_color }
       , icon_button_element<true>{ code, size, body_color }
      };
   }
}}

#endif
