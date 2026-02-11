local config = require('config')
local ambient = require('ambient')

--- @type table<Object, { x: number, y: number, z: number }>
local sLastPosByFlame = {}

--- @type table<Object, integer>
local sFlameLightIdByObj = setmetatable({}, { __mode = 'k' })

local function is_castle_basement_flame(o)
    if o == nil then return false end
    if ambient.get_current_level_num() ~= LEVEL_CASTLE then return false end
    if obj_has_behavior_id(o, id_bhvFlame) ~= 1 then return false end

    local modelId = obj_get_model_id_extended(o)
    if type(modelId) ~= 'number' then return false end

    return modelId == E_MODEL_RED_FLAME or modelId == E_MODEL_BLUE_FLAME
end

local function remove_flame_light(o)
    if o == nil then return end

    local id = sFlameLightIdByObj[o]
    if id ~= nil and id ~= 0 and le_light_exists(id) then
        le_remove_light(id)
    end
    sFlameLightIdByObj[o] = nil
    sLastPosByFlame[o] = nil
end

local function ensure_flame_light(o)
    if o == nil then return end

    local existing = sFlameLightIdByObj[o]
    if existing ~= nil and existing ~= 0 and le_light_exists(existing) then
        return
    end

    local modelId = obj_get_model_id_extended(o)
    if type(modelId) ~= 'number' then return end

    local r = config.FLAME_LIGHT_RED_R
    local g = config.FLAME_LIGHT_RED_G
    local b = config.FLAME_LIGHT_RED_B
    if modelId == E_MODEL_BLUE_FLAME then
        r = config.FLAME_LIGHT_BLUE_R
        g = config.FLAME_LIGHT_BLUE_G
        b = config.FLAME_LIGHT_BLUE_B
    end

    local id = le_add_light(
        o.oPosX, o.oPosY, o.oPosZ,
        r, g, b,
        config.FLAME_LIGHT_RADIUS, config.FLAME_LIGHT_INTENSITY
    )

    sFlameLightIdByObj[o] = id
    sLastPosByFlame[o] = { x = o.oPosX, y = o.oPosY, z = o.oPosZ }
end

local function update_flame_light(o)
    ensure_flame_light(o)
    local id = sFlameLightIdByObj[o]
    if id ~= nil and id ~= 0 and le_light_exists(id) then
        local last = sLastPosByFlame[o]
        if last == nil
            or math.abs(last.x - o.oPosX) > config.POS_EPSILON
            or math.abs(last.y - o.oPosY) > config.POS_EPSILON
            or math.abs(last.z - o.oPosZ) > config.POS_EPSILON then
            le_set_light_pos(id, o.oPosX, o.oPosY, o.oPosZ)
            sLastPosByFlame[o] = { x = o.oPosX, y = o.oPosY, z = o.oPosZ }
        end
    end
end

local function on_object_unload_flame(unloadedObject)
    if not is_castle_basement_flame(unloadedObject) then return end
    remove_flame_light(unloadedObject)
end

local function bhv_flame_light_init(o)
    if not is_castle_basement_flame(o) then return end
    ensure_flame_light(o)
end

local function bhv_flame_light_loop(o)
    if not is_castle_basement_flame(o) then return end
    update_flame_light(o)
end

hook_behavior(id_bhvFlame, OBJ_LIST_LEVEL, false, bhv_flame_light_init, bhv_flame_light_loop)

hook_event(HOOK_ON_OBJECT_UNLOAD, on_object_unload_flame)
