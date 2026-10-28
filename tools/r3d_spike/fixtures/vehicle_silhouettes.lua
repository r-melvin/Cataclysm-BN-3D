-- PRIVATE world only. Change vehicle_id to inspect another native prototype.
-- Examples: 2seater2, SmallFlier1, car_sports, car, car_hatch, suv, pickup, cube_van, bus, semi_truck.
-- Replaces the vehicle at the avatar's origin and clears a 31 x 31 display pad.
local vehicle_id = "2seater2"
local m = gapi.get_map()
local p = gapi.get_avatar():get_pos_ms()
local n = m:get_map_size()
assert(p.x >= 20 and p.y >= 20 and p.x + 20 < n and p.y + 20 < n and p.z + 5 <= 10, "Vehicle pad crosses loaded bounds")
local floor = TerId.new("t_floor"):int_id()
local empty = FurnId.new("f_null"):int_id()
for dx = -15, 15 do
 for dy = -15, 15 do
  local q = TripointBubMs.new(p.x+dx,p.y+dy,p.z)
  m:set_ter_at(q,floor)
  m:set_furn_at(q,empty)
 end
end
-- Remove the earlier appliance fixture's radio column from this disposable pad.
for z = p.z+1, p.z+5 do
 m:set_ter_at(TripointBubMs.new(p.x-8,p.y+1,z),TerId.new("t_open_air"):int_id())
end
assert(m:replace_vehicle(p,vehicle_id,{orientation=270,status=0,locks=false}), "Stand on the test vehicle's origin")
gapi.get_avatar():set_mutation(MutationBranchId.new("DEBUG_NIGHTVISION"))
print("Native vehicle silhouette ready: " .. vehicle_id .. "; exit console and wait a turn")
