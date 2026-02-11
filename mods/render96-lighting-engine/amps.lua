local config = require('config')

--- @type table<Object, { x: number, y: number, z: number }>
local sLastPosByAmp = {}

--- @type table<Object, integer>
local sAmpLightIdByObj = setmetatable({}, { __mode = 'k' })

local function is_amp(o)
    if o == nil then return false end
    return obj_has_behavior_id(o, id_bhvCirclingAmp) == 1
        or obj_has_behavior_id(o, id_bhvHomingAmp) == 1
end

local function remove_amp_light(o)
    if o == nil then return end

    local id = sAmpLightIdByObj[o]
    if id ~= nil and id ~= 0 and le_light_exists(id) then
        le_remove_light(id)
    end

    sAmpLightIdByObj[o] = nil
    sLastPosByAmp[o] = nil
end

local function ensure_amp_light(o)
    if o == nil then return end

    local existing = sAmpLightIdByObj[o]
    if existing ~= nil and existing ~= 0 and le_light_exists(existing) then
        return
    end

    local id = le_add_light(
        o.oPosX, o.oPosY, o.oPosZ,
        config.AMP_LIGHT_COLOR_R, config.AMP_LIGHT_COLOR_G, config.AMP_LIGHT_COLOR_B,
        config.AMP_LIGHT_RADIUS, config.AMP_LIGHT_INTENSITY
    )

    sAmpLightIdByObj[o] = id
    sLastPosByAmp[o] = { x = o.oPosX, y = o.oPosY, z = o.oPosZ }
end

local function update_amp_light(o)
    ensure_amp_light(o)

    local id = sAmpLightIdByObj[o]
    if id ~= nil and id ~= 0 and le_light_exists(id) then
        local last = sLastPosByAmp[o]
        if last == nil
            or math.abs(last.x - o.oPosX) > config.POS_EPSILON
            or math.abs(last.y - o.oPosY) > config.POS_EPSILON
            or math.abs(last.z - o.oPosZ) > config.POS_EPSILON then
            le_set_light_pos(id, o.oPosX, o.oPosY, o.oPosZ)
            sLastPosByAmp[o] = { x = o.oPosX, y = o.oPosY, z = o.oPosZ }
        end
    end
end

local function on_object_unload_amp(unloadedObject)
    if not is_amp(unloadedObject) then return end
    remove_amp_light(unloadedObject)
end

local function bhv_amp_light_init(o)
    if not is_amp(o) then return end
    ensure_amp_light(o)
end

local function bhv_amp_light_loop(o)
    if not is_amp(o) then return end
    update_amp_light(o)
end

hook_behavior(id_bhvCirclingAmp, OBJ_LIST_LEVEL, false, bhv_amp_light_init, bhv_amp_light_loop)
hook_behavior(id_bhvHomingAmp, OBJ_LIST_LEVEL, false, bhv_amp_light_init, bhv_amp_light_loop)

hook_event(HOOK_ON_OBJECT_UNLOAD, on_object_unload_amp)
