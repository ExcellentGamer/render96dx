local config = require('config')

--- @type table<Object, { x: number, y: number, z: number }>
local sLastPosByStar = {}

--- @type table<Object, integer>
local sLightIdByObj = setmetatable({}, { __mode = 'k' })

--- @param o Object
local function is_star_behavior(o)
    if o == nil then return false end
    return obj_has_behavior_id(o, id_bhvStar) == 1
        or obj_has_behavior_id(o, id_bhvGrandStar) == 1
        or obj_has_behavior_id(o, id_bhvSpawnedStar) == 1
        or obj_has_behavior_id(o, id_bhvSpawnedStarNoLevelExit) == 1
        or obj_has_behavior_id(o, id_bhvStarSpawnCoordinates) == 1
        or obj_has_behavior_id(o, id_bhvCelebrationStar) == 1
        or obj_has_behavior_id(o, id_bhvCcmTouchedStarSpawn) == 1
end

--- @param o Object
local function remove_star_light(o)
    if o == nil then return end

    local id = sLightIdByObj[o]
    if id ~= nil and id ~= 0 and le_light_exists(id) then
        le_remove_light(id)
    end
    sLightIdByObj[o] = nil
    sLastPosByStar[o] = nil
end

--- @param o Object
local function ensure_star_light(o)
    if o == nil then return end

    local existing = sLightIdByObj[o]
    if existing ~= nil and existing ~= 0 and le_light_exists(existing) then
        return
    end

    local id = le_add_light(
        o.oPosX, o.oPosY, o.oPosZ,
        config.STAR_LIGHT_COLOR_R, config.STAR_LIGHT_COLOR_G, config.STAR_LIGHT_COLOR_B,
        config.STAR_LIGHT_RADIUS, config.STAR_LIGHT_INTENSITY
    )

    sLightIdByObj[o] = id
    sLastPosByStar[o] = { x = o.oPosX, y = o.oPosY, z = o.oPosZ }
end

--- @param o Object
local function update_star_light(o)
    ensure_star_light(o)
    local id = sLightIdByObj[o]
    if id ~= nil and id ~= 0 and le_light_exists(id) then
        local last = sLastPosByStar[o]
        if last == nil
            or math.abs(last.x - o.oPosX) > config.POS_EPSILON
            or math.abs(last.y - o.oPosY) > config.POS_EPSILON
            or math.abs(last.z - o.oPosZ) > config.POS_EPSILON then
            le_set_light_pos(id, o.oPosX, o.oPosY, o.oPosZ)
            sLastPosByStar[o] = { x = o.oPosX, y = o.oPosY, z = o.oPosZ }
        end
    end
end

--- @param unloadedObject Object
local function on_object_unload(unloadedObject)
    if not is_star_behavior(unloadedObject) then return end
    remove_star_light(unloadedObject)
end

--- @param o Object
local function bhv_star_light_init(o)
    if not is_star_behavior(o) then return end
    ensure_star_light(o)
end

--- @param o Object
local function bhv_star_light_loop(o)
    if not is_star_behavior(o) then return end
    update_star_light(o)
end

hook_behavior(id_bhvStar, OBJ_LIST_LEVEL, false, bhv_star_light_init, bhv_star_light_loop)
hook_behavior(id_bhvGrandStar, OBJ_LIST_LEVEL, false, bhv_star_light_init, bhv_star_light_loop)
hook_behavior(id_bhvSpawnedStar, OBJ_LIST_LEVEL, false, bhv_star_light_init, bhv_star_light_loop)
hook_behavior(id_bhvSpawnedStarNoLevelExit, OBJ_LIST_LEVEL, false, bhv_star_light_init, bhv_star_light_loop)
hook_behavior(id_bhvStarSpawnCoordinates, OBJ_LIST_LEVEL, false, bhv_star_light_init, bhv_star_light_loop)
hook_behavior(id_bhvCelebrationStar, OBJ_LIST_LEVEL, false, bhv_star_light_init, bhv_star_light_loop)
hook_behavior(id_bhvCcmTouchedStarSpawn, OBJ_LIST_LEVEL, false, bhv_star_light_init, bhv_star_light_loop)

hook_event(HOOK_ON_OBJECT_UNLOAD, on_object_unload)
