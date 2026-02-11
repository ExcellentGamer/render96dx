---------- Squash and Stretch ----------

----- Value Handlers -----

function vertical_speed_scaling(m)
    local d = gMarioDetails[m.playerIndex]

    if (m.action & ACT_FLAG_AIR) ~= 0 and m.action ~= ACT_FLYING and m.action ~= ACT_SHOT_FROM_CANNON and
    m.action ~= ACT_VERTICAL_WIND then

        d.squishGoal = m.vel.y

        d.squishStrength = approach_f32(d.squishStrength, d.squishGoal, d.squishGoal, 4)

        if d.squishStrength < -35.0 then
            d.squishStrength = -35.0
        end

        vScaling = math.abs(d.squishStrength * stretchVModifierAir)

    elseif (m.action & ACT_GROUP_MASK) ~= ACT_GROUP_SUBMERGED and
    (m.action & ACT_GROUP_MASK) ~= ACT_GROUP_AUTOMATIC then

        if d.squishGoal < -75.0 then
            d.squishGoal = -75.0
        end

        if d.squishStrength == d.squishGoal then
            d.squishGoal = d.squishGoal / -2
        else
            d.squishStrength = approach_f32(d.squishStrength, d.squishGoal, 12, 12)
        end

        if d.squishGoal >= -12 and d.squishGoal <= 12 then
            d.squishGoal = 0
        end

        if d.squishStrength >= 0 then
            vScaling = (d.squishStrength * stretchVModifierGround)
        else
            vScaling = (d.squishStrength * (stretchVModifierGround * 2))
        end

    else
        d.squishStrength = 0
        d.squishGoal = 0
        vScaling = 0
    end

    return vScaling
end

function horizontal_speed_scaling(m)
    local d = gMarioDetails[m.playerIndex]

    d.stretchController = approach_f32(d.stretchController, m.forwardVel, 3, 3)

    clamp(d.stretchController, -64, 64)

    hScaling = math.abs(d.stretchController * stretchHModifier)

    return hScaling
end

----- The actual squash and stretch -----

function squash_and_stretch(m)
    local d = gMarioDetails[m.playerIndex]

    local scale = m.marioObj.header.gfx.scale

    vertical_speed_scaling(m)
    horizontal_speed_scaling(m)

    if m.squishTimer == 0 then
        if (m.action == ACT_LAVA_DEATH and not snowyTerrain and m.vel.y <= 0)
        or (m.action == ACT_STANDING_DEATH and m.prevAction == ACT_BURNING_GROUND and m.actionTimer >= 25) then
            d.scaleWhenBurnt = d.scaleWhenBurnt + 1 / 25
            scale.x = 1.0 - d.scaleWhenBurnt
            scale.y = 1.0 - d.scaleWhenBurnt
            scale.z = 1.0 - d.scaleWhenBurnt
        else
            d.scaleWhenBurnt = 0
            scale.x = 1.0 + (vScaling + hScaling) * -0.5
            scale.y = 1.0 + (vScaling + (hScaling * -0.3))
            scale.z = 1.0 + ((vScaling * -0.3) + hScaling)
        end
    end
end