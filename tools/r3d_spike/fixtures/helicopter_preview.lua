-- PRIVATE world only. Stand in/on the test vehicle; clears a central landing pad.
local m = gapi.get_map()
local p = gapi.get_avatar():get_pos_ms()
local n = m:get_map_size()
assert(p.x >= 20 and p.y >= 20 and p.x + 20 < n and p.y + 20 < n, "Helicopter fixture crosses loaded bounds")
local empty = FurnId.new("f_null"):int_id()
local floor = TerId.new("t_floor"):int_id()
for dx = -4, 4 do
 for dy = -9, 9 do
  local q = TripointBubMs.new(p.x+dx,p.y+dy,p.z)
  m:set_furn_at(q,empty)
  m:set_ter_at(q,floor)
 end
end
assert(m:replace_vehicle(p,"2seater2",{orientation=270,status=0,locks=false}),"No vehicle under the avatar, or replacement failed")
print("Native 2-seater helicopter ready with intact 8-metre rotors; radio/outer fixtures preserved.")
