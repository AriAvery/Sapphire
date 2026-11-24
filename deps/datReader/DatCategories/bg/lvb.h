#pragma once

#include "lgb.h"

struct LVB_FILE
{
  std::vector< LGB_GROUP > groups;
  std::vector< std::string > layerPaths;

  LVB_FILE( std::vector< char >& data, const LGB_FILE::AssetTypeFilter* pTypeFilter )
  {
    const LayoutSection section{ data.data(), data.size() };
    const auto base = section.chunk( "LVB1", "SCN1" );
    const auto scene = section.read< Scene_t >( base );
    const auto groupsOffset = section.table( base, scene.LayerGroups, scene.LayerGroup_Count, sizeof( LayerGroup ) );
    for( int i = 0; i < scene.LayerGroup_Count; ++i )
      LGB_FILE::readLayers( data.data(), section, groupsOffset + i * sizeof( LayerGroup ), false, pTypeFilter, groups );

    const auto paths = section.table( base, scene.LGBAssetPaths, scene.LGBAssetPath_Count, 4 );
    for( int i = 0; i < scene.LGBAssetPath_Count; ++i )
      layerPaths.push_back( section.stringAt( paths, section.read< int32_t >( paths + i * 4 ) ) );
  }
};
