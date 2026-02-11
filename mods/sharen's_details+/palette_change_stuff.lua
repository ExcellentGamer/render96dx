----- Functions -----

function set_marios_color(color, curPalette, p)
    local result = { r = 0, g = 0, b = 0 }

    result.r = lerp(color.r, curPalette.r, p)
    result.g = lerp(color.g, curPalette.g, p)
    result.b = lerp(color.b, curPalette.b, p)

    return result
end

function is_becoming_ash(m)
    if (m.action == ACT_LAVA_DEATH and m.vel.y <= 0 and not snowyTerrain)
    or (m.action == ACT_STANDING_DEATH and m.prevAction == ACT_BURNING_GROUND and m.actionTimer >= 25) then
        return true
    end
    return false
end

----- Toast (or freeze) Mario -----

function toast_or_freeze_the_mario(m)
    local d = gMarioDetails[m.playerIndex]

    if d.storedShade == nil then
        d.storedShade = {
            shadeR = m.marioBodyState.shadeR,
            shadeG = m.marioBodyState.shadeG,
            shadeB = m.marioBodyState.shadeB,
            lightR = m.marioBodyState.lightR,
            lightG = m.marioBodyState.lightG,
            lightB = m.marioBodyState.lightB,
        }
    end

    local og = d.storedShade
    local target = { shadeR = og.shadeR, shadeG = og.shadeG, shadeB = og.shadeB, lightR = og.lightR, lightG = og.lightG, lightB = og.lightB }

    -- burnOrFreezeTimer is driven elsewhere; here we only render based on its value
    local t = d.burnOrFreezeTimer
    local sign = (t > 0 and 1) or (t < 0 and -1) or 0
    local p = clamp(math.abs(t) / 60, 0, 1)

    -- smoothly approach the target blend direction and strength
    d.lerpVariable = approach_f32(d.lerpVariable, sign * p, 0.08, 0.08)

    if sign > 0 then
        -- darker / ashy (burn/cannon)
        target.shadeR = 40
        target.shadeG = 40
        target.shadeB = 40
        target.lightR = 80
        target.lightG = 60
        target.lightB = 40
    elseif sign < 0 then
        -- cold blue tint
        target.shadeR = 120
        target.shadeG = 150
        target.shadeB = 255
        target.lightR = 140
        target.lightG = 170
        target.lightB = 255
    end

    p = math.abs(d.lerpVariable)
    if p < 0.001 then
        m.marioBodyState.shadeR = og.shadeR
        m.marioBodyState.shadeG = og.shadeG
        m.marioBodyState.shadeB = og.shadeB
        m.marioBodyState.lightR = og.lightR
        m.marioBodyState.lightG = og.lightG
        m.marioBodyState.lightB = og.lightB
        return
    end

    m.marioBodyState.shadeR = clamp(math.floor(lerp(og.shadeR, target.shadeR, p) + 0.5), 0, 255)
    m.marioBodyState.shadeG = clamp(math.floor(lerp(og.shadeG, target.shadeG, p) + 0.5), 0, 255)
    m.marioBodyState.shadeB = clamp(math.floor(lerp(og.shadeB, target.shadeB, p) + 0.5), 0, 255)
    m.marioBodyState.lightR = clamp(math.floor(lerp(og.lightR, target.lightR, p) + 0.5), 0, 255)
    m.marioBodyState.lightG = clamp(math.floor(lerp(og.lightG, target.lightG, p) + 0.5), 0, 255)
    m.marioBodyState.lightB = clamp(math.floor(lerp(og.lightB, target.lightB, p) + 0.5), 0, 255)
end
