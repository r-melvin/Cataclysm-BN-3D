-- PRIVATE world only. Adds isolated sink/ranges and a six-storey native radio column.
local m = gapi.get_map()
local p = gapi.get_avatar():get_pos_ms()
local n = m:get_map_size()
assert(p.x >= 9 and p.y >= 7 and p.x + 9 < n and p.y + 7 < n and p.z + 5 <= 10, "Fixture crosses loaded bounds")
local floor = TerId.new("t_floor"):int_id()
local empty = FurnId.new("f_null"):int_id()
for dx = -9, 9 do
 for dy = -7, 7 do
  local q = TripointBubMs.new(p.x+dx,p.y+dy,p.z)
  m:set_ter_at(q,floor)
  m:set_furn_at(q,empty)
 end
end
local ids = {"f_sink", "f_oven", "f_fridge", "f_dishwasher", "f_washer"}
for i,id in ipairs(ids) do
 m:set_furn_at(TripointBubMs.new(p.x-9+i*3,p.y-4,p.z),FurnId.new(id):int_id())
 m:set_furn_at(TripointBubMs.new(p.x-9+i*3,p.y+4,p.z),FurnId.new(id):int_id())
end
for z = p.z, p.z+5 do
 m:set_ter_at(TripointBubMs.new(p.x-8,p.y+1,z),TerId.new("t_radio_tower"):int_id())
end
print("Appliances and six-storey radio support ready; native vehicles preserved.")
