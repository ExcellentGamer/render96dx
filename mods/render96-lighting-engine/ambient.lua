local config = require('config')

local M = {}

local sCurrentLevelNum = nil
local sCurrentAreaIndex = nil

local function get_level_area_ambient(levelNum, areaIndex)
    local levelAreas = config.AMBIENT_BY_LEVEL_AREA[levelNum]
    if levelAreas ~= nil then
        local ovr = levelAreas[areaIndex]
        if ovr ~= nil then
            return ovr.r, ovr.g, ovr.b
        end
    end

    local levelOvr = config.AMBIENT_BY_LEVEL[levelNum]
    if levelOvr ~= nil then
        return levelOvr.r, levelOvr.g, levelOvr.b
    end

    return config.DEFAULT_LEVEL_AMBIENT_R, config.DEFAULT_LEVEL_AMBIENT_G, config.DEFAULT_LEVEL_AMBIENT_B
end

local function update_ambient_if_needed()
    local m = gMarioStates[0]
    if m == nil or m.area == nil then return end

    local areaIndex = m.area.index
    if type(areaIndex) ~= 'number' then return end

    local levelNum = sCurrentLevelNum
    if type(levelNum) ~= 'number' then return end

    if sCurrentAreaIndex == areaIndex then return end
    sCurrentAreaIndex = areaIndex

    local r, g, b = get_level_area_ambient(levelNum, areaIndex)
    le_set_ambient_color(r, g, b)
end

function M.get_current_level_num()
    return sCurrentLevelNum
end

local function on_level_init(_, levelNum, _, _, _)
    le_set_mode(LE_MODE_AFFECT_ALL_SHADED_AND_COLORED)
    sCurrentLevelNum = levelNum
    sCurrentAreaIndex = nil

    update_ambient_if_needed()
end

local function on_update()
    update_ambient_if_needed()
end

hook_event(HOOK_ON_LEVEL_INIT, on_level_init)
hook_event(HOOK_UPDATE, on_update)

return M
