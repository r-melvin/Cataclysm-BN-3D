-- Developer fixture for a PRIVATE test world; paste into the native Lua console.
-- Replaces a 13 x 11 area with isolated, joined and wall-backed furniture.
local here = gapi.get_map()
local center = gapi.get_avatar():get_pos_ms()
local floor = TerId.new("t_floor"):int_id()
local wall = TerId.new("t_wall"):int_id()
local empty_furniture = FurnId.new("f_null"):int_id()
local map_size = here:get_map_size()
assert(center.x >= 6 and center.x + 6 < map_size and center.y >= 5 and center.y + 5 < map_size,
  "Furniture fixture crosses the loaded map boundary")
local kinds = { "f_trashcan", "f_chair", "f_locker", "f_table" }
for dx = -6, 6 do
  for dy = -5, 5 do
    local pos = TripointBubMs.new(center.x + dx, center.y + dy, center.z)
    here:set_ter_at(pos, floor)
    here:set_furn_at(pos, empty_furniture)
  end
  here:set_ter_at(TripointBubMs.new(center.x + dx, center.y - 4, center.z), wall)
end
for index, id in ipairs(kinds) do
  local x = center.x - 7 + index * 3
  local furniture = FurnId.new(id):int_id()
  here:set_furn_at(TripointBubMs.new(x, center.y - 3, center.z), furniture)
  here:set_furn_at(TripointBubMs.new(x, center.y - 1, center.z), furniture)
  here:set_furn_at(TripointBubMs.new(x, center.y + 2, center.z), furniture)
  here:set_furn_at(TripointBubMs.new(x + 1, center.y + 2, center.z), furniture)
end
print("Furniture fixture ready: bin, chair, locker, table; wall-backed, isolated, joined.")
