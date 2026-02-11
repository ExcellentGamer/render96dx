--- @param m MarioState
function minor_details(m)
    local d = gMarioDetails[m.playerIndex]

    local dYaw = m.intendedYaw - m.faceAngle.y
    local intendedMag = m.intendedMag / 32
    local animInfo = m.marioObj.header.gfx.animInfo

    if (m.action == ACT_WALKING and m.actionTimer == 3) then
        m.marioObj.header.gfx.angle.x = math.min(m.marioObj.oMarioWalkingPitch + m.marioBodyState.torsoAngle.x * 0.35, deg_to_hex(40))
        m.marioObj.header.gfx.angle.z = m.marioBodyState.torsoAngle.z * 0.35
    end

    if (m.action & ACT_FLAG_AIR) ~= 0 and not (gNoTiltActions[m.action] or (m.action & ACT_GROUP_MASK) == ACT_GROUP_CUTSCENE) then
        d.tiltController.x = approach_s32(d.tiltController.x, s16(6144 * intendedMag * coss(dYaw)) / 5, deg_to_hex(1), deg_to_hex(1))
        d.tiltController.z = approach_s32(d.tiltController.z, s16(-4096 * intendedMag * sins(dYaw)) / 4, deg_to_hex(1), deg_to_hex(1))

        -- read this as:
        -- d.tiltController.x < 0 ? d.tiltController.x : 0
        m.marioObj.header.gfx.angle.x = math.min(d.tiltController.x, 0)
        m.marioObj.header.gfx.angle.z = d.tiltController.z
    else
        d.tiltController.x = 0
        d.tiltController.z = 0
    end

    if m.action == ACT_DIVE then
        d.tiltController.x = approach_s32(d.tiltController.x, atan2s(m.forwardVel, -m.vel.y), deg_to_hex(7.5), deg_to_hex(7.5))
        m.marioObj.header.gfx.angle.x = d.tiltController.x
    end

    if m.action == ACT_FELL_OFF_STAGE then
        m.marioObj.header.gfx.angle.x = m.marioObj.header.gfx.angle.x + deg_to_hex(17.5)
        m.marioObj.header.gfx.angle.y = m.marioObj.header.gfx.angle.y + deg_to_hex(17.5)
    end

    if (m.flags & MARIO_VANISH_CAP) ~= 0 and m.marioBodyState.modelState > 0xFF then
        m.marioBodyState.modelState = m.marioBodyState.modelState + opacityController
    end

    if gOpenHandsAnimations[animInfo.animID] and animInfo.animFrame < animInfo.curAnim.loopEnd - gOpenHandsAnimations[animInfo.animID] then
        m.marioBodyState.handState = MARIO_HAND_OPEN
    elseif animInfo.animID == MARIO_ANIM_THROW_LIGHT_OBJECT or animInfo.animID == MARIO_ANIM_GROUND_THROW or animInfo.animID == MARIO_ANIM_START_WALLKICK then
        m.marioBodyState.handState = MARIO_HAND_RIGHT_OPEN
    elseif animInfo.animID == MARIO_ANIM_SINGLE_JUMP and handGrowthOnJump then
        m.marioBodyState.punchState = (0 << 6) | 3
    end
end