-- Developer fixture for a PRIVATE test world; paste into the native Lua console.
-- Keeps existing vehicles; replaces nearby terrain and furniture.
local m = gapi.get_map()
local p = gapi.get_avatar():get_pos_ms()
local n = m:get_map_size()
assert(p.x >= 9 and p.y >= 7 and p.x + 9 < n and p.y + 7 < n, "Readability fixture crosses map boundary")
local floor = TerId.new("t_floor"):int_id()
local none = FurnId.new("f_null"):int_id()
for dx = -9, 9 do
 for dy = -7, 7 do
  local q = TripointBubMs.new(p.x+dx,p.y+dy,p.z)
  m:set_ter_at(q,floor)
  m:set_furn_at(q,none)
 end
end
local wall = TerId.new("t_brick_wall"):int_id()
for dx = -8, 8 do
 m:set_ter_at(TripointBubMs.new(p.x+dx,p.y-6,p.z),wall)
end
for dy = -6, 6 do
 m:set_ter_at(TripointBubMs.new(p.x+8,p.y+dy,p.z),TerId.new("t_wall_wood"):int_id())
end
m:set_ter_at(TripointBubMs.new(p.x-2,p.y-6,p.z),TerId.new("t_window_domestic"):int_id())
m:set_ter_at(TripointBubMs.new(p.x+3,p.y-6,p.z),TerId.new("t_door_c"):int_id())
local ids = {"f_sink", "f_oven", "f_fridge", "f_dishwasher", "f_washer"}
for i,id in ipairs(ids) do
 m:set_furn_at(TripointBubMs.new(p.x-9+i*3,p.y-4,p.z),FurnId.new(id):int_id())
 m:set_furn_at(TripointBubMs.new(p.x-9+i*3,p.y+4,p.z),FurnId.new(id):int_id())
end
print("Readability fixture ready: sink, oven, fridge, dishwasher, washer; brick and wood walls.")
