#!/usr/bin/env python3
"""Copy các model KayKit Medieval Hexagon (CC0) cần cho thế giới mở vào assets/models/world.
Dùng: python3 tools/prepare_world.py <thư_mục Assets/gltf của pack> assets/models/world"""
import os, shutil, sys
src, dst = sys.argv[1], sys.argv[2]
os.makedirs(dst, exist_ok=True)
files = {
  'tiles/base': ['hex_grass', 'hex_water'],
  'tiles/roads': ['hex_road_' + c for c in 'ABCDEFGHIJKLM'],
  'tiles/coast': ['hex_coast_A', 'hex_coast_B', 'hex_coast_C', 'hex_coast_D', 'hex_coast_E'],
  'decoration/nature': ['tree_single_A', 'tree_single_B', 'trees_A_large', 'trees_A_medium', 'trees_B_large', 'trees_B_medium',
                        'rock_single_A', 'rock_single_B', 'rock_single_C', 'mountain_A_grass_trees', 'mountain_B_grass_trees',
                        'mountain_C_grass', 'hills_A_trees', 'hill_single_A', 'cloud_big', 'cloud_small', 'waterlily_A', 'waterplant_A'],
  'decoration/props': ['barrel', 'crate_A_big', 'sack', 'tent', 'wheelbarrow', 'target', 'weaponrack', 'flag_red', 'flag_blue', 'bucket_water', 'resource_lumber'],
  'buildings/blue': ['building_home_A_blue', 'building_castle_blue', 'building_well_blue', 'building_tower_A_blue'],
  'buildings/red': ['building_tavern_red', 'building_home_B_red', 'building_blacksmith_red'],
  'buildings/green': ['building_market_green', 'building_church_green', 'building_home_A_green'],
  'buildings/yellow': ['building_windmill_yellow', 'building_watermill_yellow', 'building_lumbermill_yellow', 'building_home_B_yellow'],
  'buildings/neutral': ['building_bridge_A', 'building_stage_A', 'building_grain', 'fence_wood_straight'],
}
for folder, names in files.items():
    for n in names:
        for ext in ('.gltf', '.bin'):
            p = os.path.join(src, folder, n + ext)
            if os.path.exists(p): shutil.copy(p, dst)
            else: print('thiếu', p)
shutil.copy(os.path.join(src, 'tiles/base/hexagons_medieval.png'), dst)
print('xong:', len(os.listdir(dst)), 'file')
