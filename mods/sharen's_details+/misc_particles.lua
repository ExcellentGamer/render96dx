------ Frame Perfect Particles ------

function FP_sparkles_init(o)
    o.oFlags = OBJ_FLAG_UPDATE_GFX_POS_AND_ANGLE
    o.oAnimState = -1
    obj_set_billboard(o)

    o.oPosX = o.oPosX + math.random(-60, 60)
    o.oPosY = o.oPosY + math.random(-30, 70)
    o.oPosZ = o.oPosZ + math.random(-60, 60)
end

function FP_sparkles_loop(o)
    if o.oTimer > 9 then
        o.activeFlags = ACTIVE_FLAG_DEACTIVATED
    end

    o.oAnimState = o.oAnimState + 1
    o.oPosY = o.oPosY + 9
    obj_scale(o, (10 - o.oTimer) / 5)
end

id_bhvFPSparkle = hook_behavior(nil, OBJ_LIST_DEFAULT, true, FP_sparkles_init, FP_sparkles_loop)

function send_frame_perfect_feedback(m)
    local d = gMarioDetails[m.playerIndex]

    if d.framePerfect == 1 and (m.action == ACT_FORWARD_ROLLOUT or m.action == ACT_JUMP_KICK
    or m.action == ACT_WALL_KICK_AIR) then
        d.feedbackTimer = 4
        d.framePerfect = 0
    else
        d.framePerfect = 0
    end

    if d.feedbackTimer > 0 then
        spawn_non_sync_object(id_bhvFPSparkle, E_MODEL_SPARKLES_ANIMATION, m.pos.x, m.pos.y, m.pos.z, nil)
        d.feedbackTimer = d.feedbackTimer - 1
    end
end

function frame_perfect_chances(m)
    local d = gMarioDetails[m.playerIndex]

    if m.playerIndex == 0 then
        if m.action == ACT_DIVE_SLIDE or m.action == ACT_AIR_HIT_WALL then
            m.actionTimer = m.actionTimer + 1

            if m.actionTimer == 1 then
                d.framePerfect = 1
            else
                d.framePerfect = 0
            end
        elseif m.action == ACT_WALKING and m.controller.stickMag > 48.0 and m.actionTimer == 3 and
        m.forwardVel >= 29.0 then
            d.framePerfect = 1
        else
            send_frame_perfect_feedback(m)
        end
    end
end

------ Burning Smoke ------

function burn_smoke_init(o)
    o.oFlags = OBJ_FLAG_UPDATE_GFX_POS_AND_ANGLE
    o.oVelY = math.random(12, 18)
    obj_set_billboard(o)
    o.oPosX = o.oPosX - math.random(-40, 40)
    o.oPosY = o.oPosY + 40
    o.oPosZ = o.oPosZ - math.random(-40, 40)
end

function burn_smoke_loop(o)
    o.oPosY = o.oPosY + o.oVelY

    if o.oTimer >= math.random(10, 15) then
        obj_scale(o, o.header.gfx.scale.x - 0.1)
    end

    if o.header.gfx.scale.x <= 0 then
        obj_mark_for_deletion(o)
    end
end

hook_behavior(id_bhvBlackSmokeMario, OBJ_LIST_UNIMPORTANT, true, burn_smoke_init, burn_smoke_loop)

------ Cartoon Star ------

function ver_cartoon_star_init(o)
    o.oFlags = OBJ_FLAG_UPDATE_GFX_POS_AND_ANGLE
    o.oCollisionParticleUnkF4 = 0.35
    cur_obj_scale(o.oCollisionParticleUnkF4)
    o.oAngleVelRoll = deg_to_hex(math.random(10, 20))
    o.oVelY = o.oVelY + 25
    o.oAnimState = 4
end

function hori_cartoon_star_init(o)
    o.oFlags = OBJ_FLAG_UPDATE_GFX_POS_AND_ANGLE
    o.oCollisionParticleUnkF4 = 0.35
    cur_obj_scale(o.oCollisionParticleUnkF4)
    o.oAngleVelRoll = deg_to_hex(math.random(10, 20))
    o.oForwardVel = 25
    o.oVelY = 30
    o.oAnimState = 4
    o.oWallHitboxRadius = 100 * o.oCollisionParticleUnkF4
end

function cartoon_star_loop(o)
    cur_obj_move_using_fvel_and_gravity()

    o.oForwardVel = approach_f32(o.oForwardVel, 0, 0.5, 0.5)
    o.oVelY = o.oVelY - 4

    o.oFloorHeight = find_floor_height(o.oPosX, o.oPosY, o.oPosZ)

    if o.oPosY + o.oVelY <= o.oFloorHeight then
        o.oVelY = o.oVelY / -1.5
        o.oPosY = o.oFloorHeight + o.oVelY
    end

    o.oWallHitboxRadius = 100 * o.oCollisionParticleUnkF4

    if cur_obj_resolve_wall_collisions() ~= 0 then
        o.oMoveAngleYaw = o.oMoveAngleYaw + deg_to_hex(180)
    end

    o.oFaceAngleYaw = gMarioStates[0].area.camera.yaw
    o.oFaceAnglePitch = gLakituState.oldPitch
    o.oFaceAngleRoll = o.oFaceAngleRoll + o.oAngleVelRoll

    cur_obj_scale(o.oCollisionParticleUnkF4)
    o.oCollisionParticleUnkF4 = o.oCollisionParticleUnkF4 - 0.015

    if o.oCollisionParticleUnkF4 <= 0 then
        obj_mark_for_deletion(o)
    end
end

hook_behavior(id_bhvWallTinyStarParticle, OBJ_LIST_UNIMPORTANT, true, ver_cartoon_star_init, cartoon_star_loop)
hook_behavior(id_bhvPoundTinyStarParticle, OBJ_LIST_UNIMPORTANT, true, hori_cartoon_star_init, cartoon_star_loop)

------ Jump Dust ------

function jumping_dust_init(o)
    o.oFlags = OBJ_FLAG_UPDATE_GFX_POS_AND_ANGLE
    obj_set_billboard(o)

    o.oPosX = o.oPosX + 20 * sins(o.oMoveAngleYaw) * coss(o.oMoveAnglePitch)
    o.oPosY = o.oPosY + 20 * sins(o.oMoveAnglePitch)
    o.oPosZ = o.oPosZ + 20 * coss(o.oMoveAngleYaw) * coss(o.oMoveAnglePitch)

    o.oWallHitboxRadius = 20 * o.header.gfx.scale.x
    o.oAnimState = -1
end

function jumping_dust_loop(o)
    o.oForwardVel = approach_f32(o.oForwardVel, 0, 0.725, 0.725)
    o.oWallHitboxRadius = 20 * o.header.gfx.scale.x

    o.oVelX = o.oForwardVel * sins(o.oMoveAngleYaw) * coss(o.oMoveAnglePitch)
    o.oVelY = o.oForwardVel * sins(o.oMoveAnglePitch)
    o.oVelZ = o.oForwardVel * coss(o.oMoveAngleYaw) * coss(o.oMoveAnglePitch)

    if cur_obj_resolve_wall_collisions() == 0 then
        obj_move_xyz(o, o.oVelX, o.oVelY, o.oVelZ)
    end

    o.oFloorHeight = find_floor_height(o.oPosX, o.oPosY + 100, o.oPosZ)

    if math.abs(o.oPosY - o.oFloorHeight) < 40 then
        o.oPosY = o.oFloorHeight
    end

    if o.oTimer >= 6 then
        obj_scale(o, o.header.gfx.scale.x - 0.1)
    end

    if o.header.gfx.scale.x <= 0 then
        obj_mark_for_deletion(o)
    end

    o.oAnimState = o.oAnimState + 1
end

id_bhvJumpDust = hook_behavior(nil, OBJ_LIST_UNIMPORTANT, false, jumping_dust_init, jumping_dust_loop)

function handle_dust_on_set_mario_action(m)
    local d = gMarioDetails[m.playerIndex]
    maxParticles = 8

    if m.action == ACT_WALL_KICK_AIR then
        for i = 0, maxParticles do
            spawn_non_sync_object(id_bhvJumpDust, E_MODEL_SMOKE, m.pos.x, m.pos.y, m.pos.z, function (o)
                o.oMoveAnglePitch = deg_to_hex(360 / maxParticles * i)
                o.oMoveAngleYaw = m.faceAngle.y - deg_to_hex(90)
                o.oForwardVel = 7.5 + m.forwardVel / 10
                obj_scale(o, 0.8)
            end)
        end
        d.jumpDustTimer = math.floor(m.vel.y / 15)
    end

    if m.action == ACT_LONG_JUMP then
        d.jumpDustTimer = math.floor(math.abs(m.forwardVel / 4))
    elseif (m.action & ACT_FLAG_AIR) ~= 0 and d.grounded and m.vel.y > 20 then
        d.jumpDustTimer = math.floor(m.vel.y / 15)
    end
end

function handle_dust_mario_update(m)
    local d = gMarioDetails[m.playerIndex]
    local maxParticles = math.min(12, 5 + math.floor(math.abs(math.min(m.vel.y, d.prevYVel)) / 25))

    if m.pos.y - m.floorHeight > 0 then
        if d.grounded then
            d.grounded = false
        end
    else
        if not d.grounded then
            if m.peakHeight - m.pos.y > 1150 and m.hurtCounter > 0 then
                m.particleFlags = m.particleFlags | PARTICLE_HORIZONTAL_STAR
            end

            if ((m.action & ACT_GROUP_MASK) ~= ACT_GROUP_SUBMERGED and (m.action & ACT_GROUP_MASK) ~= ACT_GROUP_AUTOMATIC) and
            ((m.floor.type == SURFACE_BURNING) or (m.vel.y <= -45 or d.prevYVel <= -45)) then
                -- read this as:
                -- m.floor.type == SURFACE_BURNING ? E_MODEL_RED_FLAME : E_MODEL_SMOKE
                -- m.floor.type == SURFACE_BURNING ? 2 : 0.8
                local particleModel = m.floor.type == SURFACE_BURNING and E_MODEL_RED_FLAME or E_MODEL_SMOKE
                local particleScale = m.floor.type == SURFACE_BURNING and 2.25 or 0.8

                if not (m.floor.type == SURFACE_BURNING and snowyTerrain) then
                    for i = 0, maxParticles do
                        spawn_non_sync_object(id_bhvJumpDust, particleModel, m.pos.x, m.floorHeight, m.pos.z, function (o)
                            o.oMoveAngleYaw = deg_to_hex(360 / maxParticles * i)
                            o.oMoveAnglePitch = 0
                            obj_scale(o, particleScale)
                            o.oForwardVel = 7.5 + math.abs(math.min(m.vel.y, d.prevYVel)) / 20
                        end)
                    end
                end
            end
            d.grounded = true
        end
    end

    if d.jumpDustTimer > 0 and (m.vel.y > 20 or m.action == ACT_LONG_JUMP) then
        spawn_non_sync_object(id_bhvJumpDust, E_MODEL_SMOKE, m.pos.x, m.pos.y, m.pos.z, function (o)
            o.oMoveAngleYaw = deg_to_hex(math.random(0, 360))
            obj_scale(o, 0.725)
        end)
        d.jumpDustTimer = d.jumpDustTimer - 1
    end
end

------ Speed Stuff ------

--- Speed Rings Particle ---

function speed_ring_init(o)
    o.oFlags = (OBJ_FLAG_UPDATE_GFX_POS_AND_ANGLE | OBJ_FLAG_SET_FACE_ANGLE_TO_MOVE_ANGLE)
    o.oOpacity = 0xFF
    obj_scale(o, 1.1)
    o.oAngleVelRoll = deg_to_hex(math.random(20, 30))
end

function speed_ring_loop(o)
    o.oVelX = o.oForwardVel * sins(o.oMoveAngleYaw) * coss(o.oMoveAnglePitch)
    o.oVelY = o.oForwardVel * sins(o.oMoveAnglePitch)
    o.oVelZ = o.oForwardVel * coss(o.oMoveAngleYaw) * coss(o.oMoveAnglePitch)

    if cur_obj_resolve_wall_collisions() == 0 then
        obj_move_xyz(o, o.oVelX, o.oVelY, o.oVelZ)
    end

    obj_scale(o, o.header.gfx.scale.x + 0.1)

    o.oMoveAngleRoll = o.oMoveAngleRoll + o.oAngleVelRoll

    o.oOpacity = o.oOpacity - 0xFF / 5

    if o.oOpacity <= 0 then
        obj_mark_for_deletion(o)
    end
end

id_bhvSpeedRing = hook_behavior(nil, OBJ_LIST_UNIMPORTANT, false, speed_ring_init, speed_ring_loop)

--- Motion Lines Particle ---

function motion_line_init(o)
    local m = gMarioStates[o.oOwner]
    local yMultiplier = math.floor(m.marioBodyState.headPos.y + 60 - m.marioObj.header.gfx.pos.y) < 0 and -1 or 1

    o.oFlags = (OBJ_FLAG_UPDATE_GFX_POS_AND_ANGLE | OBJ_FLAG_SET_FACE_ANGLE_TO_MOVE_ANGLE)
    o.oOpacity = 0x86
    obj_scale(o, math.random(2, 4) / 10)
    o.header.gfx.scale.z = math.random(4, 6) / 10

    o.oHomeX = math.random(-60, 60)
    o.oHomeY = math.random(0, math.abs(math.floor(m.marioBodyState.headPos.y + 60 - m.marioObj.header.gfx.pos.y))) * yMultiplier
    o.oHomeZ = math.random(-30, 30)

    obj_set_pos_relative_to_mario_fake_pos(o, m, o.oHomeX, o.oHomeY, o.oHomeZ)
end

---@param o Object
function motion_line_loop(o)
    local m = gMarioStates[o.oOwner]
    local d = gMarioDetails[m.playerIndex]
    local pitch = m.forwardVel > 0 and atan2s(m.forwardVel, -m.vel.y) or atan2s(-m.forwardVel, -m.vel.y)

    obj_set_pos_relative_to_mario_fake_pos(o, m, o.oHomeX, o.oHomeY, o.oHomeZ)

    o.oMoveAngleYaw = atan2s(m.vel.z, m.vel.x)
    o.oMoveAnglePitch = d.grounded and -find_floor_slope(m, 0) or pitch

    o.oOpacity = o.oOpacity - 0x86 / 10
    o.header.gfx.scale.z = o.header.gfx.scale.z - 0.05

    if o.oOpacity <= 0 or o.header.gfx.scale.z <= 0 then
        obj_mark_for_deletion(o)
    end
end

id_bhvMotionLine = hook_behavior(nil, OBJ_LIST_UNIMPORTANT, false, motion_line_init, motion_line_loop)

--- Particle Spawners ---

function spawn_speed_particles(m)
    local mGfx = m.marioObj.header.gfx
    local d = gMarioDetails[m.playerIndex]
    local pitch = m.forwardVel > 0 and atan2s(m.forwardVel, -m.vel.y) or atan2s(-m.forwardVel, -m.vel.y)

    if (math.abs(m.forwardVel) >= 64 or (m.vel.y <= -80 and not d.grounded)) and speedParticleTimer == 3 then
        spawn_non_sync_object(id_bhvSpeedRing, E_MODEL_SPEED_RING, mGfx.pos.x, mGfx.pos.y + 60, mGfx.pos.z, function(o)
            o.oMoveAngleYaw = m.faceAngle.y
            o.oMoveAnglePitch = d.grounded and -find_floor_slope(m, 0) or atan2s(m.forwardVel, -m.vel.y)
            o.oForwardVel = math.max(m.forwardVel, m.vel.y) / 4
            o.oOwner = m.playerIndex
        end)
    end

    if math.abs(m.forwardVel) >= 50 and (speedParticleTimer == 1 or speedParticleTimer == 3) then
        spawn_non_sync_object(id_bhvMotionLine, E_MODEL_MOTION_LINE, mGfx.pos.x, mGfx.pos.y, mGfx.pos.z, function(o)
            o.oMoveAngleYaw = atan2s(m.vel.z, m.vel.x)
            o.oMoveAnglePitch = d.grounded and -find_floor_slope(m, 0) or pitch
        end)
    end
end
