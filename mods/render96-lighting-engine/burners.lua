local config = require('config')

--- @type table<Object, integer[]>
local sBurnerLightIdsByObj = setmetatable({}, { __mode = 'k' })

local function is_burner_emitter(o)
    if o == nil then return false end
    return obj_has_behavior_id(o, id_bhvFlamethrower) == 1
        or obj_has_behavior_id(o, id_bhvFireSpitter) == 1
end

local function is_burner_firing(o)
    if o == nil then return false end

    if obj_has_behavior_id(o, id_bhvFlamethrower) == 1 then
        return o.oAction == 1
    end

    if obj_has_behavior_id(o, id_bhvFireSpitter) == 1 then
        return o.oAction == FIRE_SPITTER_ACT_SPIT_FIRE
    end

    return false
end

local function remove_burner_lights(o)
    if o == nil then return end
    local ids = sBurnerLightIdsByObj[o]
    if ids ~= nil then
        for i = 1, #ids do
            local id = ids[i]
            if id ~= nil and id ~= 0 and le_light_exists(id) then
                le_remove_light(id)
            end
        end
    end
    sBurnerLightIdsByObj[o] = nil
end

local function get_burner_reach(o)
    if o == nil then return 0 end

    if obj_has_behavior_id(o, id_bhvFlamethrower) == 1 then
        local flameVel = 95
        if o.oBehParams2ndByte == 2 then
            flameVel = 50
        end

        local duration = o.oFlameThowerUnk110 or 0
        local reach = flameVel * duration
        if reach < 0 then reach = 0 end
        if reach > 1200 then reach = 1200 end
        return reach
    end

    if obj_has_behavior_id(o, id_bhvFireSpitter) == 1 then
        return config.FIRE_SPITTER_REACH
    end

    return 0
end

local function ensure_burner_lights(o, r, g, b)
    if o == nil then return end

    local ids = sBurnerLightIdsByObj[o]
    if ids == nil then
        ids = {}
        sBurnerLightIdsByObj[o] = ids
    end

    for i = 1, config.BURNER_LIGHT_COUNT do
        local existing = ids[i]
        if existing == nil or existing == 0 or not le_light_exists(existing) then
            ids[i] = le_add_light(
                o.oPosX, o.oPosY, o.oPosZ,
                r, g, b,
                config.BURNER_LIGHT_RADIUS, config.BURNER_LIGHT_INTENSITY
            )
        end
    end
end

local function update_burner_lights(o)
    if o == nil then return end

    if not is_burner_firing(o) then
        remove_burner_lights(o)
        return
    end

    local modelId = obj_get_model_id_extended(o)
    local useBlue = (type(modelId) == 'number' and modelId == E_MODEL_BLUE_FLAME)

    local r = config.BURNER_LIGHT_COLOR_R
    local g = config.BURNER_LIGHT_COLOR_G
    local b = config.BURNER_LIGHT_COLOR_B
    if useBlue then
        r = config.BURNER_LIGHT_BLUE_COLOR_R
        g = config.BURNER_LIGHT_BLUE_COLOR_G
        b = config.BURNER_LIGHT_BLUE_COLOR_B
    end

    ensure_burner_lights(o, r, g, b)
    local ids = sBurnerLightIdsByObj[o]
    if ids == nil then return end

    local yaw = o.oMoveAngleYaw
    local dx = sins(yaw)
    local dz = coss(yaw)

    local baseY = o.oPosY + (o.oGraphYOffset or 0)
    if obj_has_behavior_id(o, id_bhvFireSpitter) == 1 then
        baseY = o.oPosY + 40
    end

    local reach = get_burner_reach(o)
    local offsets = { 0, reach * 0.5, reach }

    for i = 1, config.BURNER_LIGHT_COUNT do
        local id = ids[i]
        if id ~= nil and id ~= 0 and le_light_exists(id) then
            local dist = offsets[i] or offsets[#offsets]
            le_set_light_pos(id, o.oPosX + dx * dist, baseY, o.oPosZ + dz * dist)
        end
    end
end

local function on_object_unload_burner(unloadedObject)
    if not is_burner_emitter(unloadedObject) then return end
    remove_burner_lights(unloadedObject)
end

local function bhv_burner_light_init(o)
    if not is_burner_emitter(o) then return end
    update_burner_lights(o)
end

local function bhv_burner_light_loop(o)
    if not is_burner_emitter(o) then return end
    update_burner_lights(o)
end

hook_behavior(id_bhvFlamethrower, OBJ_LIST_LEVEL, false, bhv_burner_light_init, bhv_burner_light_loop)
hook_behavior(id_bhvFireSpitter, OBJ_LIST_LEVEL, false, bhv_burner_light_init, bhv_burner_light_loop)

hook_event(HOOK_ON_OBJECT_UNLOAD, on_object_unload_burner)
