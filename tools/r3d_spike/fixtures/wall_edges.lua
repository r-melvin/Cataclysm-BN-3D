-- Developer fixture for a PRIVATE world. Replaces a 19 x 17 terrain/furniture area.
local m = gapi.get_map()
local a = gapi.get_avatar()
local p = a:get_pos_ms()
local n = m:get_map_size()
assert(p.x >= 9 and p.y >= 8 and p.x + 9 < n and p.y + 8 < n, "Wall edge fixture crosses the loaded map boundary")
local grass = TerId.new("t_grass"):int_id()
local floor = TerId.new("t_floor"):int_id()
local wall = TerId.new("t_wall"):int_id()
local none = FurnId.new("f_null"):int_id()
for dx = -9, 9 do
  for dy = -8, 8 do
    local q = TripointBubMs.new(p.x + dx, p.y + dy, p.z)
    m:set_ter_at(q, grass)
    m:set_furn_at(q, none)
    if dx >= -7 and dx <= 7 and dy >= -6 and dy <= 6 then
      m:set_ter_at(q, (dx == -7 or dx == 7 or dy == -6 or dy == 6) and wall or floor)
    end
  end
end
m:set_ter_at(TripointBubMs.new(p.x - 3, p.y - 6, p.z), TerId.new("t_window_domestic"):int_id())
m:set_ter_at(TripointBubMs.new(p.x + 2, p.y - 6, p.z), TerId.new("t_door_o"):int_id())
m:set_ter_at(TripointBubMs.new(p.x + 7, p.y + 2, p.z), TerId.new("t_window_frame"):int_id())
m:set_ter_at(TripointBubMs.new(p.x - 7, p.y + 2, p.z), TerId.new("t_door_c"):int_id())
for dy = -5, -1 do
  m:set_ter_at(TripointBubMs.new(p.x - 2, p.y + dy, p.z), wall)
end
m:set_furn_at(TripointBubMs.new(p.x - 5, p.y - 5, p.z), FurnId.new("f_locker"):int_id())
m:set_furn_at(TripointBubMs.new(p.x + 6, p.y - 3, p.z), FurnId.new("f_oven"):int_id())
print("Wall edge fixture ready: four exterior faces, shared partition, openings and fitted cabinets.")
