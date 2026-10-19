-- Developer fixture for a PRIVATE test world, run from BN's Lua console.
-- Replaces a 17 x 13 area around the avatar with a floor and an opening-state wall.
-- West to east: intact/open/broken/empty windows, closed/open/broken/frame doors.
-- Native terrain IDs keep their movement, perception and interaction behavior.
local here = gapi.get_map()
local center = gapi.get_avatar():get_pos_ms()
local floor = TerId.new("t_floor"):int_id()
local wall = TerId.new("t_wall"):int_id()
local empty_furniture = FurnId.new("f_null"):int_id()
local map_size = here:get_map_size()
assert(center.x >= 8 and center.x + 8 < map_size and center.y >= 6 and center.y + 6 < map_size,
  "Opening fixture crosses the loaded map boundary")
local states = {
  "t_window", "t_window_open", "t_window_frame", "t_window_empty",
  "t_door_c", "t_door_o", "t_door_b", "t_door_frame",
}
for dx = -8, 8 do
  for dy = -6, 6 do
    local pos = TripointBubMs.new(center.x + dx, center.y + dy, center.z)
    here:set_ter_at(pos, floor)
    here:set_furn_at(pos, empty_furniture)
  end
  here:set_ter_at(TripointBubMs.new(center.x + dx, center.y - 3, center.z), wall)
end
for index, id in ipairs(states) do
  local pos = TripointBubMs.new(center.x - 9 + index * 2, center.y - 3, center.z)
  here:set_ter_at(pos, TerId.new(id):int_id())
end
print("Opening fixture ready: windows intact/open/broken/empty, doors closed/open/broken/frame.")
