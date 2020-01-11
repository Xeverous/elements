/*=============================================================================
   Copyright (c) 2016-2019 Joel de Guzman

   Distributed under the MIT License [ https://opensource.org/licenses/MIT ]
=============================================================================*/
#include <elements/element/tile.hpp>
#include <elements/support/context.hpp>

namespace cycfi { namespace elements
{
   namespace
   {
      struct layout_info
      {
         float min, max, stretch, alloc;
      };
   }

   ////////////////////////////////////////////////////////////////////////////
   // Vertical Tiles
   ////////////////////////////////////////////////////////////////////////////
   view_limits vtile_element::limits(basic_context const& ctx) const
   {
      view_limits limits{ { 0.0, 0.0 }, { full_extent, 0.0 } };
      for (std::size_t i = 0; i != size();  ++i)
      {
         auto el = at(i).limits(ctx);

         limits.min.y += el.min.y;
         limits.max.y += el.max.y;
         clamp_min(limits.min.x, el.min.x);
         clamp_max(limits.max.x, el.max.x);
      }

      clamp_min(limits.max.x, limits.min.x);
      clamp_max(limits.max.y, full_extent);
      return limits;
   }

   namespace
   {
      // Compute the best fit for all elements
      void allocate(
         double size, double max_stretch, double total
       , std::vector<layout_info>& info)
      {
         double extra = size - total;
         for (int i = 1; i < 10; ++i) // loop no more than 10 times
         {
            double remove_stretch = 0.0;
            total = 0.0;
            for (auto& info : info)
            {
               if (info.alloc < info.max)       // This element can grow
               {
                  info.alloc += extra * info.stretch / max_stretch;
                  if (info.alloc >= info.max)   // We exceeded its max
                  {
                     info.alloc = info.max;
                     remove_stretch -= info.stretch;
                  }
               }
               total += info.alloc;
            }
            extra = size - total;
            max_stretch -= remove_stretch;

            // return if there's no more room to grow or if we can't stretch anymore
            if (max_stretch < 1 || extra < 0.5)
               return;
         }
      }
   }

   void vtile_element::layout(context const& ctx)
   {
      _left = ctx.bounds.left;
      _right = ctx.bounds.right;
      _tiles.resize(size()+1);

      double const height = ctx.bounds.height();

      // Collect min, max, and stretch information from each element. Also,
      // accumulate the maximum stretch (max_stretch) for later. Initially set the
      // allocation sizes of each element to its minimum.
      double max_stretch = 0.0;
      float total = 0.0;
      std::vector<layout_info> info(size());
      for (std::size_t i = 0; i != size(); ++i)
      {
         auto& elem = at(i);
         auto limits = elem.limits(ctx);
         info[i].stretch = elem.stretch().y;
         total += (info[i].alloc = info[i].min = limits.min.y);
         info[i].max = limits.max.y;
         if (info[i].alloc < info[i].max) // Can grow?
            max_stretch += info[i].stretch;
      }

      // Compute the best fit for all elements
      allocate(height, max_stretch, total, info);

      // Now we have the final layout. We can now layout the individual
      // elements.
      double curr = ctx.bounds.top;
      auto iter = _tiles.begin();
      std::size_t i = 0;
      for (auto const& info : info)
      {
         *iter++ = curr;
         auto prev = curr;
         curr += info.alloc;

         auto& elem = at(i++);
         rect ebounds = { _left, float(prev), _right, float(curr) };
         elem.layout(context{ ctx, &elem, ebounds });
      }
      *iter = curr;
   }

   rect vtile_element::bounds_of(context const& /* ctx */, std::size_t index) const
   {
      if (index >= _tiles.size())
         return {};
      return rect{ _left, _tiles[index], _right, _tiles[index+1] };
   }

   ////////////////////////////////////////////////////////////////////////////
   // Horizontal Tiles
   ////////////////////////////////////////////////////////////////////////////
   view_limits htile_element::limits(basic_context const& ctx) const
   {
      view_limits limits{ { 0.0, 0.0 }, { 0.0, full_extent } };
      for (std::size_t i = 0; i != size();  ++i)
      {
         auto el = at(i).limits(ctx);

         limits.min.x += el.min.x;
         limits.max.x += el.max.x;
         clamp_min(limits.min.y, el.min.y);
         clamp_max(limits.max.y, el.max.y);
      }

      clamp_min(limits.max.y, limits.min.y);
      clamp_max(limits.max.x, full_extent);
      return limits;
   }

   void htile_element::layout(context const& ctx)
   {
      _top = ctx.bounds.top;
      _bottom = ctx.bounds.bottom;
      _tiles.resize(size()+1);

      double const width = ctx.bounds.width();

      // Collect min, max, and stretch information from each element. Also,
      // accumulate the maximum stretch (max_stretch) for later. Initially
      // set the allocation sizes of each element to its minimum.
      double max_stretch = 0.0;
      double total = 0.0;
      std::vector<layout_info> info(size());
      for (std::size_t i = 0; i != size(); ++i)
      {
         auto& elem = at(i);
         auto limits = elem.limits(ctx);
         info[i].stretch = elem.stretch().x;
         total += (info[i].alloc = info[i].min = limits.min.x);
         info[i].max = limits.max.x;
         if (info[i].alloc < info[i].max) // Can stretch?
            max_stretch += info[i].stretch;
      }

      // Compute the best fit for all elements
      allocate(width, max_stretch, total, info);

      // Now we have the final layout. We can now layout the individual
      // elements.
      double curr = ctx.bounds.left;
      auto iter = _tiles.begin();
      std::size_t i = 0;
      for (auto const& info : info)
      {
         *iter++ = curr;
         auto prev = curr;
         curr += info.alloc;

         auto& elem = at(i++);
         rect ebounds = { float(prev), _top, float(curr), _bottom };
         elem.layout(context{ ctx, &elem, ebounds });
      }
      *iter = curr;
   }

   rect htile_element::bounds_of(context const& /* ctx */, std::size_t index) const
   {
      if (index >= _tiles.size())
         return {};
      return rect{ _tiles[index], _top, _tiles[index + 1], _bottom };
   }
}}
