---------- OBJECTS ----------

----- Extra Hurt Feedback -----

function e_hurt_fb_init(o)
    o.oFlags = OBJ_FLAG_UPDATE_GFX_POS_AND_ANGLE

    obj_scale_random(o, 0.15, 0.45)
    o.oVelY = math.random(12.0, 34.0)
    o.oForwardVel = math.random(29.0, 42.0)
    o.oFaceAngleYaw = math.random(-0x8000, 0x8000)
    o.oMoveAngleYaw = o.oFaceAngleYaw
end

function e_hurt_fb_loop(o)
    o.oPosY = o.oPosY + o.oVelY
    if o.header.gfx.scale.x <= 0.0 then
        o.activeFlags = ACTIVE_FLAG_DEACTIVATED
    end

    o.oVelY = o.oVelY - 4
    cur_obj_move_xz_using_fvel_and_yaw()

    o.oFaceAngleYaw = o.oFaceAngleYaw + math.random(0x1000, 0x2000)
    o.oFaceAngleRoll = o.oFaceAngleRoll + math.random(0x1000, 0x2000)
    o.oFaceAnglePitch = o.oFaceAnglePitch + math.random(0x1000, 0x2000)
    obj_scale(o, (o.header.gfx.scale.x - 0.05))
end

id_bhvEHurtFB = hook_behavior(nil, OBJ_LIST_UNIMPORTANT, true, e_hurt_fb_init, e_hurt_fb_loop)

----- Extra Heal Feedback -----

function e_heal_fb_init(o)
    local m = gMarioStates[o.oOwner]
    o.oFlags = OBJ_FLAG_UPDATE_GFX_POS_AND_ANGLE

    obj_scale_random(o, 0.15, 0.45)
    o.oRotationDiameter = 90
    o.oFaceAnglePitch = 0
    o.oFaceAngleRoll = 0
end

--- @param o Object
function e_heal_fb_loop(o)
    local m = gMarioStates[o.oOwner]

    o.oHomeX = m.pos.x
    o.oHomeZ = m.pos.z

    o.oVelY = o.oVelY + 4

    o.oRotationDiameter = o.oRotationDiameter + 5

    o.oPosX = o.oHomeX + sins(o.oMoveAngleYaw) * o.oRotationDiameter / 2
    o.oPosY = m.pos.y + o.oVelY
    o.oPosZ = o.oHomeZ + coss(o.oMoveAngleYaw) * o.oRotationDiameter / 2

    o.oMoveAngleYaw = o.oMoveAngleYaw + 0x2000
    o.oFaceAngleYaw = mario_obj_angle_to_object(m, o)

    if o.oTimer > 30 then
        obj_scale(o, (o.header.gfx.scale.x - 0.05))
    end

    if o.header.gfx.scale.x <= 0.0 then
        o.activeFlags = ACTIVE_FLAG_DEACTIVATED
    end
end

id_bhvEHealFB = hook_behavior(nil, OBJ_LIST_UNIMPORTANT, true, e_heal_fb_init, e_heal_fb_loop)

---------- MARIO FUNCTIONS ----------

hurtFBTimer = 0
healFBTimer = 0

----- Spawn stuff -----

--- @param m MarioState
function spawn_extra_health_fb(m)
    local d = gMarioDetails[m.playerIndex]

    curHPWedges = math.floor(m.health / 256)

    if curHPWedges ~= d.prevWedgesAmount then
        if curHPWedges < d.prevWedgesAmount then
            hurtFBTimer = d.prevWedgesAmount - curHPWedges
        else
            healFBTimer = curHPWedges - d.prevWedgesAmount
        end
        d.prevWedgesAmount = curHPWedges
    end

    if hurtFBTimer > 0 then
        for i = 0, hurtFBTimer do
            spawn_non_sync_object(id_bhvEHurtFB, E_MODEL_STAR, m.pos.x, m.pos.y + 50, m.pos.z, nil)
            spawn_non_sync_object(id_bhvEHurtFB, E_MODEL_STAR, m.pos.x, m.pos.y + 50, m.pos.z, nil)
            hurtFBTimer = hurtFBTimer - 1
        end
    end

    if healFBTimer > 0 then
        for i = 0, healFBTimer do
            spawn_non_sync_object(id_bhvEHealFB, E_MODEL_HEART, m.pos.x, m.pos.y, m.pos.z, function(o)
                o.oOwner = m.playerIndex
            end)
            healFBTimer = healFBTimer - 1
        end
    end
end

function render_others_hp()
    return
end