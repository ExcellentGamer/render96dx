local config = require('config')

--- @type table<Object, { x: number, y: number, z: number }>
local sLastPosByObj = {}

--- @type table<Object, integer>
local sLightIdByObj = setmetatable({}, { __mode = 'k' })

local function is_fire_hazard(o)
    if o == nil then return false end

    return obj_has_behavior_id(o, id_bhvBouncingFireball) == 1
        or obj_has_behavior_id(o, id_bhvBouncingFireballFlame) == 1
        or obj_has_behavior_id(o, id_bhvFlyguyFlame) == 1
        or obj_has_behavior_id(o, id_bhvKoopaShellFlame) == 1
        or obj_has_behavior_id(o, id_bhvSmallPiranhaFlame) == 1
        or obj_has_behavior_id(o, id_bhvVolcanoFlames) == 1
        or obj_has_behavior_id(o, id_bhvLllRotatingHexFlame) == 1
        or obj_has_behavior_id(o, id_bhvFlameBowser) == 1
        or obj_has_behavior_id(o, id_bhvBlueBowserFlame) == 1
end

local function uses_blue_fire_color(o)
    if o == nil then return false end

    if obj_has_behavior_id(o, id_bhvBlueBowserFlame) == 1 then
        return true
    end

    local modelId = obj_get_model_id_extended(o)
    if type(modelId) == 'number' and modelId == E_MODEL_BLUE_FLAME then
        return true
    end

    return false
end

local function remove_fire_hazard_light(o)
    if o == nil then return end

    local id = sLightIdByObj[o]
    if id ~= nil and id ~= 0 and le_light_exists(id) then
        le_remove_light(id)
    end

    sLightIdByObj[o] = nil
    sLastPosByObj[o] = nil
end

local function ensure_fire_hazard_light(o)
    if o == nil then return end

    local existing = sLightIdByObj[o]
    if existing ~= nil and existing ~= 0 and le_light_exists(existing) then
        return
    end

    local r = config.FIRE_HAZARD_LIGHT_COLOR_R
    local g = config.FIRE_HAZARD_LIGHT_COLOR_G
    local b = config.FIRE_HAZARD_LIGHT_COLOR_B
    if uses_blue_fire_color(o) then
        r = config.FIRE_HAZARD_LIGHT_BLUE_COLOR_R
        g = config.FIRE_HAZARD_LIGHT_BLUE_COLOR_G
        b = config.FIRE_HAZARD_LIGHT_BLUE_COLOR_B
    end

    local id = le_add_light(
        o.oPosX, o.oPosY, o.oPosZ,
        r, g, b,
        config.FIRE_HAZARD_LIGHT_RADIUS, config.FIRE_HAZARD_LIGHT_INTENSITY
    )

    sLightIdByObj[o] = id
    sLastPosByObj[o] = { x = o.oPosX, y = o.oPosY, z = o.oPosZ }
end

local function update_fire_hazard_light(o)
    ensure_fire_hazard_light(o)

    local id = sLightIdByObj[o]
    if id ~= nil and id ~= 0 and le_light_exists(id) then
        local last = sLastPosByObj[o]
        if last == nil
            or math.abs(last.x - o.oPosX) > config.POS_EPSILON
            or math.abs(last.y - o.oPosY) > config.POS_EPSILON
            or math.abs(last.z - o.oPosZ) > config.POS_EPSILON then
            le_set_light_pos(id, o.oPosX, o.oPosY, o.oPosZ)
            sLastPosByObj[o] = { x = o.oPosX, y = o.oPosY, z = o.oPosZ }
        end
    end
end

local function on_object_unload_fire_hazard(unloadedObject)
    if not is_fire_hazard(unloadedObject) then return end
    remove_fire_hazard_light(unloadedObject)
end

local function bhv_fire_hazard_light_init(o)
    if not is_fire_hazard(o) then return end
    ensure_fire_hazard_light(o)
end

local function bhv_fire_hazard_light_loop(o)
    if not is_fire_hazard(o) then return end
    update_fire_hazard_light(o)
end

hook_behavior(id_bhvBouncingFireball, OBJ_LIST_LEVEL, false, bhv_fire_hazard_light_init, bhv_fire_hazard_light_loop)
hook_behavior(id_bhvBouncingFireballFlame, OBJ_LIST_LEVEL, false, bhv_fire_hazard_light_init, bhv_fire_hazard_light_loop)
hook_behavior(id_bhvFlyguyFlame, OBJ_LIST_LEVEL, false, bhv_fire_hazard_light_init, bhv_fire_hazard_light_loop)
hook_behavior(id_bhvKoopaShellFlame, OBJ_LIST_LEVEL, false, bhv_fire_hazard_light_init, bhv_fire_hazard_light_loop)
hook_behavior(id_bhvSmallPiranhaFlame, OBJ_LIST_LEVEL, false, bhv_fire_hazard_light_init, bhv_fire_hazard_light_loop)
hook_behavior(id_bhvVolcanoFlames, OBJ_LIST_LEVEL, false, bhv_fire_hazard_light_init, bhv_fire_hazard_light_loop)
hook_behavior(id_bhvLllRotatingHexFlame, OBJ_LIST_LEVEL, false, bhv_fire_hazard_light_init, bhv_fire_hazard_light_loop)
hook_behavior(id_bhvFlameBowser, OBJ_LIST_LEVEL, false, bhv_fire_hazard_light_init, bhv_fire_hazard_light_loop)
hook_behavior(id_bhvBlueBowserFlame, OBJ_LIST_LEVEL, false, bhv_fire_hazard_light_init, bhv_fire_hazard_light_loop)

hook_event(HOOK_ON_OBJECT_UNLOAD, on_object_unload_fire_hazard)
