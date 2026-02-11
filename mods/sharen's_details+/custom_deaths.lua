ACT_LAVA_DEATH = allocate_mario_action(ACT_GROUP_CUTSCENE | ACT_FLAG_AIR | ACT_FLAG_INTANGIBLE)
ACT_FELL_OFF_STAGE = allocate_mario_action(ACT_GROUP_CUTSCENE | ACT_FLAG_AIR | ACT_FLAG_INTANGIBLE)
ACT_FROZEN = allocate_mario_action(ACT_GROUP_SUBMERGED | ACT_FLAG_STATIONARY | ACT_FLAG_WATER_OR_TEXT | ACT_FLAG_SWIMMING | ACT_FLAG_SWIMMING_OR_FLYING | ACT_FLAG_INTANGIBLE)

--- @param m MarioState
function act_lava_death(m)
    local d = gMarioDetails[m.playerIndex]

    local step = perform_air_step(m, 0)

    if step == AIR_STEP_HIT_WALL then
        mario_bonk_reflection(m, 0)
    elseif step == AIR_STEP_LANDED and snowyTerrain then
        if math.abs(m.vel.y * 0.4) > 6 and m.actionState ~= 1 then
            play_mario_heavy_landing_sound(m, SOUND_ACTION_TERRAIN_BODY_HIT_GROUND)
            m.vel.y = math.abs(m.vel.y * 0.4)
            mario_set_forward_vel(m, m.forwardVel * 0.5)
        else
            m.actionState = 1
            m.vel.y = 0
            m.forwardVel = 0
        end
    end

    local savedPos = m.pos.y
    local savedVel = m.vel.y

    if not snowyTerrain then
        m.vel.y = m.vel.y + 1
        m.forwardVel = approach_f32(m.forwardVel, 0, 1.5, 1.5)
        mario_set_forward_vel(m, m.forwardVel)

        if m.vel.y <= 0 then
            m.vel.y = 0
            m.actionTimer = m.actionTimer + 1
            play_sound_if_no_flag(m, SOUND_OBJ_ENEMY_DEFEAT_SHRINK, MARIO_ACTION_SOUND_PLAYED)

            if m.marioObj.header.gfx.scale.x <= 0 and m.actionState == 0 then
                play_sound(SOUND_GENERAL_FLAME_OUT, m.marioObj.header.gfx.cameraToObject)
                set_mario_particle_flags(m, PARTICLE_MIST_CIRCLE, 0)
                m.actionState = 1
                m.actionTimer = 0
                m.marioObj.header.gfx.node.flags = m.marioObj.header.gfx.node.flags & ~GRAPH_RENDER_ACTIVE
            elseif m.actionState == 1 then
                if m.actionTimer <= 3 then
                    set_camera_shake_from_hit(SHAKE_SMALL_DAMAGE)
                end
                if m.actionTimer >= 15 then
                    common_death_handler(m, m.marioObj.header.gfx.animInfo.animID,
                        m.marioObj.header.gfx.animInfo.animFrame)
                    m.pos.y = savedPos
                end
            else
                if m.actionTimer & 1 ~= 0 then
                    spawn_non_sync_object(id_bhvBlackSmokeMario, E_MODEL_BURN_SMOKE, m.pos.x, m.pos.y, m.pos.z, nil)
                end
            end
        end
    else
        m.forwardVel = approach_f32(m.forwardVel, 0, 0.35, 0.35)
        m.vel.y = m.vel.y - 2

        m.marioObj.header.gfx.animInfo.animFrame = d.frameBeforeSquishOrFreeze

        m.actionTimer = m.actionTimer + 1

        if m.actionTimer >= 45 then
            common_death_handler(m, m.marioObj.header.gfx.animInfo.animID, m.marioObj.header.gfx.animInfo.animFrame)
            -- read this as:
            -- m.actionState == 1 ? 0 : savedVel
            m.vel.y = m.actionState == 1 and 0 or savedVel
            m.pos.y = savedPos
            m.marioObj.header.gfx.pos.y = savedPos
        end
    end

    m.marioBodyState.eyeState = MARIO_EYES_DEAD
    if m.playerIndex == 0 then
        lock_camera()
    end

    return 0
end

--- @param m MarioState
function act_standing_death(m)
    local animFrame = m.marioObj.header.gfx.animInfo.animFrame

    if (m.input & INPUT_IN_POISON_GAS) ~= 0 then
        set_mario_action(m, ACT_SUFFOCATION, 0)
    end

    m.actionTimer = m.actionTimer + 1

    play_character_sound_if_no_flag(m, CHAR_SOUND_DYING, MARIO_MARIO_SOUND_PLAYED)
    common_death_handler(m, MARIO_ANIM_DYING_FALL_OVER, 80)

    if m.prevAction == ACT_BURNING_GROUND then
        if m.actionTimer >= 25 then
            play_sound_if_no_flag(m, SOUND_OBJ_ENEMY_DEFEAT_SHRINK, MARIO_ACTION_SOUND_PLAYED)

            if m.marioObj.header.gfx.scale.x <= 0 and m.actionState == 0 then
                play_sound(SOUND_GENERAL_FLAME_OUT, m.marioObj.header.gfx.cameraToObject)
                set_mario_particle_flags(m, PARTICLE_MIST_CIRCLE, 0)
                m.actionState = 1
                m.actionTimer = 0
                m.marioObj.header.gfx.node.flags = m.marioObj.header.gfx.node.flags & ~GRAPH_RENDER_ACTIVE
            elseif m.actionState == 1 then
                if m.actionTimer <= 3 then
                    set_camera_shake_from_hit(SHAKE_SMALL_DAMAGE)
                end
                if m.actionTimer >= 15 then
                    common_death_handler(m, MARIO_ANIM_DYING_FALL_OVER, animFrame)
                end
            else
                if m.actionTimer & 1 ~= 0 then
                    spawn_non_sync_object(id_bhvBlackSmokeMario, E_MODEL_BURN_SMOKE, m.pos.x, m.pos.y, m.pos.z, nil)
                end
            end
        end
    elseif animFrame == 77 then
        play_mario_landing_sound(m, SOUND_ACTION_TERRAIN_BODY_HIT_GROUND)
    end
    return 0
end

--- @param m MarioState
function act_fell_off_stage(m)
    local mGraphic = m.marioObj.header.gfx
    local animFrame = mGraphic.animInfo.animFrame

    play_character_sound_if_no_flag(m, CHAR_SOUND_WAAAOOOW, MARIO_MARIO_SOUND_PLAYED)
    set_mario_animation(m, MARIO_ANIM_AIRBORNE_ON_STOMACH)

    m.actionTimer = m.actionTimer + 1

    m.forwardVel = approach_f32(m.forwardVel, 0, 0.025, 0.025)
    mario_set_forward_vel(m, m.forwardVel)

    mGraphic.pos.x = mGraphic.pos.x + m.vel.x
    m.vel.y = m.vel.y - 8
    mGraphic.pos.y = mGraphic.pos.y + m.vel.y
    mGraphic.pos.z = mGraphic.pos.z + m.vel.z

    local savedPos = { x = m.pos.x, y = m.pos.y, z = m.pos.z }
    local savedFakePos = { x = mGraphic.pos.x, y = mGraphic.pos.y, z = mGraphic.pos.z }
    local savedSpeed = m.vel.y

    if m.actionTimer >= 50 then
        common_death_handler(m, MARIO_ANIM_AIRBORNE_ON_STOMACH, animFrame)
        vec3f_copy(m.pos, savedPos)
        vec3f_copy(mGraphic.pos, savedFakePos)
        m.vel.y = savedSpeed
    end

    m.marioBodyState.handState = MARIO_HAND_OPEN
    if m.playerIndex == 0 then
        lock_camera()
    end

    return 0
end

--- @param m MarioState
function act_squished(m)
    if m == nil then
        return true
    end

    -- read this as:
    -- m.ceilHeight - m.floorHeight < 0 ? 0 : m.ceilHeight - m.floorHeight
    local spaceUnderCeil = m.ceilHeight - m.floorHeight < 0 and 0 or m.ceilHeight - m.floorHeight
    local surfAngle
    local underSteepSurf = false

    if m.actionState == 0 then
        if spaceUnderCeil > 160 then
            m.squishTimer = 0
            m.marioObj.header.gfx.node.flags = m.marioObj.header.gfx.node.flags | GRAPH_RENDER_ACTIVE
            set_mario_action(m, ACT_IDLE, 0)
            return 0
        end

        m.squishTimer = 0xFF

        if spaceUnderCeil >= 10.1 then
            local squishAmount = spaceUnderCeil / 160
            vec3f_set(m.marioObj.header.gfx.scale, 2.0 - squishAmount, squishAmount, 2.0 - squishAmount)
        else
            if (m.flags & MARIO_METAL_CAP) == 0 and m.invincTimer == 0 then
                m.hurtCounter = ((m.flags & MARIO_CAP_ON_HEAD) ~= 0 and 12 or 18) + m.hurtCounter
                play_character_sound_if_no_flag(m, CHAR_SOUND_ATTACKED, MARIO_MARIO_SOUND_PLAYED)
            end

            vec3f_set(m.marioObj.header.gfx.scale, 1.8, 0.05, 1.8)
            queue_rumble_data_mario(m, 10, 80)
            m.actionState = 1
        end
    elseif m.actionState == 1 then
        if spaceUnderCeil >= 30 then
            m.actionState = 2
        end
    elseif m.actionState == 2 then
        m.actionTimer = m.actionTimer + 1

        if m.actionTimer >= 15 then
            if m.health < 0x100 then
                if m.playerIndex ~= 0 then
                    m.health = 0x100
                end
                common_death_handler(m, m.marioObj.header.gfx.animInfo.animID, m.marioObj.header.gfx.animInfo.animFrame)
            elseif m.hurtCounter == 0 then
                m.squishTimer = 30
                m.marioObj.header.gfx.node.flags = m.marioObj.header.gfx.node.flags | GRAPH_RENDER_ACTIVE
                set_mario_action(m, ACT_IDLE, 0)
            end
        end
    end

    m.marioObj.header.gfx.animInfo.animFrame = gMarioDetails[m.playerIndex].frameBeforeSquishOrFreeze
    m.marioObj.header.gfx.animInfo.animAccel = 0

    if m.floor ~= nil and m.floor.normal.y < 0.5 then
        surfAngle = atan2s(m.floor.normal.z, m.floor.normal.x)
        underSteepSurf = true
    end

    if m.ceil ~= nil and m.ceil.normal.y > -0.5 then
        surfAngle = atan2s(m.ceil.normal.z, m.ceil.normal.x)
        underSteepSurf = true
    end

    if underSteepSurf then
        m.vel.x = sins(surfAngle) * 10.0
        m.vel.y = 0
        m.vel.z = coss(surfAngle) * 10.0

        local step = perform_ground_step(m)

        if step == GROUND_STEP_LEFT_GROUND then
            m.squishTimer = 0
            set_mario_action(m, ACT_IDLE, 0)
            return 0
        end
    end

    m.actionArg = m.actionArg + 1

    if m.actionArg > 225 and (m.actionArg & 1) ~= 0 then
        m.marioObj.header.gfx.node.flags = m.marioObj.header.gfx.node.flags & ~GRAPH_RENDER_ACTIVE
    else
        m.marioObj.header.gfx.node.flags = m.marioObj.header.gfx.node.flags | GRAPH_RENDER_ACTIVE
    end

    if m.actionArg > 300 then
        if m.playerIndex ~= 0 then
            m.health = 0x100
        else
            m.health = 0xFF
            m.hurtCounter = 0
            m.actionState = 2
            m.marioObj.header.gfx.node.flags = m.marioObj.header.gfx.node.flags & ~GRAPH_RENDER_ACTIVE
        end
    end

    stop_and_set_height_to_floor(m)
    return 0
end

--- @param m MarioState
function act_frozen(m)
    local d = gMarioDetails[m.playerIndex]
    local mGraphic = m.marioObj.header.gfx

    m.vel.y = approach_f32(m.vel.y, 4, 0.25, 0.25)
    m.forwardVel = approach_f32(m.forwardVel, 0, 0.5, 0.5)
    mario_set_forward_vel(m, m.forwardVel)

    perform_water_step(m)
    local savedPos = m.pos.y
    local savedFakePos = { x = mGraphic.pos.x, y = mGraphic.pos.y, z = mGraphic.pos.z }
    local savedFakeAngle = { x = mGraphic.angle.x, y = mGraphic.angle.y, z = mGraphic.angle.z }
    local savedSpeed = { x = m.vel.x, y = m.vel.y, z = m.vel.z }

    m.actionTimer = m.actionTimer + 1

    m.marioObj.header.gfx.animInfo.animFrame = d.frameBeforeSquishOrFreeze
    m.marioObj.header.gfx.animInfo.animAccel = 0

    if m.actionTimer >= 45 then
        common_death_handler(m, m.marioObj.header.gfx.animInfo.animID, m.marioObj.header.gfx.animInfo.animFrame)
        m.pos.y = savedPos
        vec3f_copy(mGraphic.pos, savedFakePos)
        vec3f_copy(mGraphic.angle, savedFakeAngle)
        vec3f_copy(m.vel, savedSpeed)
    end

    m.marioBodyState.eyeState = MARIO_EYES_DEAD
    if m.playerIndex == 0 then
        lock_camera()
    end
    float_surface_gfx(m)
    set_swimming_at_surface_particles(m, PARTICLE_WAVE_TRAIL)

    return 0
end

hook_mario_action(ACT_LAVA_DEATH, act_lava_death)
hook_mario_action(ACT_STANDING_DEATH, act_standing_death)
hook_mario_action(ACT_FELL_OFF_STAGE, act_fell_off_stage)
hook_mario_action(ACT_SQUISHED, act_squished)
hook_mario_action(ACT_FROZEN, act_frozen)

textSpeed = 0
textAlpha = 0
textY = 0
textBounceCount = 0
playSoundTimer = 0
easterEggDeathTexts = { "You suck!", "skull emoji", "MISS!", "just git gud bro", "skill issue", "big L", "you kinda bad bro", "No skill?", "do better next time"}
randomNumber = 0

function too_bad()
    local m = gMarioStates[0]

    local deathText = "TOO BAD!"
    if randomNumber >= 90 and randomNumber <= (89 + #easterEggDeathTexts) then
        deathText = easterEggDeathTexts[randomNumber - 89] or "TOO BAD!"
    end
    djui_hud_set_font(FONT_MENU)
    djui_hud_set_resolution(RESOLUTION_N64)

    local gravity = 4
    local screenBottom = djui_hud_get_screen_height() - 60
    local scale = 0.9

    local red = 0xBC
    local green = 0
    local blue = 0

    if (gDeathActions[m.action] or (m.action == ACT_SQUISHED and m.health == 0xFF)) then
        if not mario_can_bubble(m) then
            fade_volume_scale(0, 0, 15)
        end

        if playSoundTimer == 0 then
            textSpeed = 0
            textAlpha = 0
            textY = 0
            textBounceCount = 0

            if m.action == ACT_FELL_OFF_STAGE then
                run_audio(gSounds.SOUND_DEATH_FALL, m, 1.85)
            elseif (m.action == ACT_LAVA_DEATH and snowyTerrain) or m.action == ACT_FROZEN then
                run_audio(gSounds.SOUND_DEATH_FROZEN, m, 1.85)
            else
                run_audio(gSounds.SOUND_DEATH_GENERIC, m, 1.85)
            end
            randomNumber = math.random(0, 89 + #easterEggDeathTexts)

        elseif playSoundTimer == math.floor(DEATH_THEME_DELAY) and not mario_can_bubble(m) then
            run_audio_stream(gSounds.THEME_TOO_BAD, 44100, 1.0, 1)

        elseif playSoundTimer > math.floor(DEATH_THEME_DELAY) and not mario_can_bubble(m) then
            textAlpha = approach_s32(textAlpha, 0xFF, 0xFF / 20, 0xFF / 20)

            textSpeed = textSpeed + gravity
            textY = textY + textSpeed

            if textY > screenBottom then
                textY = screenBottom
                if textBounceCount < 2 then
                    textSpeed = -20 / 2 ^ textBounceCount
                    textBounceCount = textBounceCount + 1
                end
            end
        end
        playSoundTimer = playSoundTimer + 1
    else
        textAlpha = approach_s32(textAlpha, 0, 0xFF / 10, 0xFF / 10)

        playSoundTimer = 0
        if textAlpha == 0 then
            textY = 0
            textBounceCount = 0
            textSpeed = 0
        end
    end

    local yMax = screenBottom
    if textY > yMax then textY = yMax end
    if textY < -200 then textY = -200 end

    djui_hud_set_color(red, green, blue, textAlpha)
    djui_hud_print_text(deathText, djui_hud_get_screen_width() / 2 - (djui_hud_measure_text(deathText) / 2) * scale, textY, scale)
end
