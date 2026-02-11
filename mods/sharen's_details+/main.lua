-- name: Sharen's Details+ v1.0.2
-- description: Adds a variety of small details and minor useful features to spice up the game! \n\nAuthor: Sharen

---------- Functions ----------

function clamp(x, min, max)
    if x < min then return min end
    if x > max then return max end
    return x
end

function lerp(min, max, p)
    return min + (max - min) * clamp(p, 0, 1)
end

function s16(x)
    x = (math.floor(x) & 0xFFFF)
    if x >= 32768 then return x - 65536 end
    return x
end

function deg_to_hex(x)
    return x * 0x10000 / 360
end

function hex_to_deg(x)
    return x * 360 / 0x10000
end

function lock_camera()
    local m = gMarioStates[0]

    gLakituState.posHSpeed = 0
    gLakituState.posVSpeed = 0
    vec3f_copy(gLakituState.goalFocus, m.pos)
end

function run_audio(sound, mState, volume)
    local audio = audio_sample_load("sound/" .. sound)
    if audio == nil or audio.loaded ~= true then
        return
    end
    audio_sample_play(audio, mState.marioObj.header.gfx.pos, volume)
end

function run_audio_stream(sound, baseFreq, speed, volume)
    local audio = audio_stream_load("sound/" .. sound)
    if audio == nil or audio.loaded ~= true then
        return
    end
    if speed == nil then
        speed = 1.0
    end
    audio_stream_set_frequency(audio, speed)
    audio_stream_play(audio, false, volume)
end

function obj_set_pos_relative_to_mario_fake_pos(o, m, dleft, dy, dforward)
    local facingZ = coss(m.faceAngle.y)
    local facingX = sins(m.faceAngle.y)

    local dz = dforward * facingZ - dleft * facingX
    local dx = dforward * facingX + dleft * facingZ

    o.oPosX = m.marioObj.header.gfx.pos.x + dx
    o.oPosY = m.marioObj.header.gfx.pos.y + dy
    o.oPosZ = m.marioObj.header.gfx.pos.z + dz
end

---------- Custom Tables, Variables, etc ----------

gMarioDetails = {}

for i = 0, (MAX_PLAYERS - 1) do
    gMarioDetails[i] = {}

    local d = gMarioDetails[i]

    d.squishStrength = 0
    d.squishGoal = 0
    d.stretchController = 0
    d.framePerfect = 0
    d.feedbackTimer = 0
    d.prevWedgesAmount = 8
    d.storedColors = {
        pants = {r = 0, g = 0, b = 0},
        shirt = {r = 0, g = 0, b = 0},
        gloves = {r = 0, g = 0, b = 0},
        shoes = {r = 0, g = 0, b = 0},
        hair = {r = 0, g = 0, b = 0},
        skin = {r = 0, g = 0, b = 0},
        cap = {r = 0, g = 0, b = 0},
    }
    d.burnOrFreezeTimer = 0
    d.lerpVariable = 0
    d.smokeTimer = 0
    d.scaleWhenBurnt = 0
    d.tiltController = {x = 0, z = 0}
    d.frameBeforeSquishOrFreeze = 0
    d.jumpDustTimer = 0
    d.grounded = true
    d.prevYVel = 0
    d.playedSound = true
end

define_custom_obj_fields({
    oOwner = 'u32',
    oOffset = 'u32',
    oRotationDiameter = 'u32',
    oIndex = 'u32'
})

gNoTiltActions = {
    [ACT_VERTICAL_WIND] = true,
    [ACT_SHOT_FROM_CANNON] = true,
    [ACT_SOFT_BONK] = true,
    [ACT_BACKWARD_AIR_KB] = true,
    [ACT_HARD_BACKWARD_AIR_KB] = true,
    [ACT_THROWN_BACKWARD] = true,
    [ACT_FORWARD_AIR_KB] = true,
    [ACT_HARD_FORWARD_AIR_KB] = true,
    [ACT_THROWN_FORWARD] = true,
    [ACT_FLYING] = true,
    [ACT_STEEP_JUMP] = true,
    [ACT_GETTING_BLOWN] = true,
    [ACT_RIDING_HOOT] = true,
    [ACT_WATER_JUMP] = true,
    [ACT_HOLD_WATER_JUMP] = true,
    [ACT_GROUND_POUND] = true,
    [ACT_CRAZY_BOX_BOUNCE] = true,
    [ACT_SLIDE_KICK] = true,
}

gDeathActions = {
    [ACT_STANDING_DEATH] = true,
    [ACT_DEATH_ON_BACK] = true,
    [ACT_DEATH_ON_STOMACH] = true,
    [ACT_SUFFOCATION] = true,
    [ACT_ELECTROCUTION] = true,
    [ACT_QUICKSAND_DEATH] = true,
    [ACT_LAVA_DEATH] = true,
    [ACT_FELL_OFF_STAGE] = true,
    [ACT_DROWNING] = true,
    [ACT_WATER_DEATH] = true,
    [ACT_FROZEN] = true,
    [ACT_EATEN_BY_BUBBA] = true,
}

-- The values in this table represent how many frames away from the animation's last frame you must be for Mario to curl his hands back into fists again.
-- So if the last frame of an anim is 36, and the value for that anim is 4, then at frame 32 Mario will curl his hands.
-- This means animations with a value of -1 have Mario always have his hands open.
gOpenHandsAnimations = {
    [MARIO_ANIM_BACKFLIP] = -1,
    [MARIO_ANIM_TRIPLE_JUMP] = -1,
    [MARIO_ANIM_DOUBLE_JUMP_RISE] = -1,
    [MARIO_ANIM_DOUBLE_JUMP_FALL] = -1,
    [MARIO_ANIM_DIVE] = -1,
    [MARIO_ANIM_SLIDE_DIVE] = -1,
    [MARIO_ANIM_TRIPLE_JUMP_LAND] = 11,
    [MARIO_ANIM_SLIDE_KICK] = -1,
    [MARIO_ANIM_SLIDEFLIP] = -1,
    [MARIO_ANIM_FAST_LONGJUMP] = -1,
    [MARIO_ANIM_SLOW_LONGJUMP] = -1,
    [MARIO_ANIM_DYING_ON_BACK] = -1,
    [MARIO_ANIM_DYING_ON_STOMACH] = -1,
    [MARIO_ANIM_LAND_ON_STOMACH] = 6,
    [MARIO_ANIM_FALL_OVER_BACKWARDS] = 6,
    [MARIO_ANIM_FORWARD_KB] = 3,
    [MARIO_ANIM_BACKWARD_KB] = 5,
    [MARIO_ANIM_WALK_PANTING] = -1,
    [MARIO_ANIM_GROUND_BONK] = 4,
    [MARIO_ANIM_AIR_FORWARD_KB] = -1,
    [MARIO_ANIM_BEING_GRABBED] = -1,
    [MARIO_ANIM_AIRBORNE_ON_STOMACH] = -1,
}

gSounds = {
    THEME_TOO_BAD = "too_bad.mp3",

    SOUND_DEATH_GENERIC = "overly_dramatic_piano.mp3",
    SOUND_DEATH_FALL = "fall_to_void.mp3",
    SOUND_DEATH_FROZEN = "frozen.mp3",

    SOUND_LONG_TRIPLE_JUMP = "long_or_triplejump.mp3",
    SOUND_DOUBLE_JUMP = "double_jump.mp3",
    SOUND_FLIP_JUMP = "back_or_sideflip.mp3",
}

E_MODEL_SPEED_RING = smlua_model_util_get_id("speedring_geo")
E_MODEL_MOTION_LINE = smlua_model_util_get_id("motionline_geo")

DEATH_THEME_DELAY = 0.75 * 30
DEATH_THEME_DURATION = 1.6 * 30 + DEATH_THEME_DELAY

stretchVModifierAir = 5.5 * 0.001
stretchVModifierGround = 2.5 * 0.001
stretchHModifier = 2.8 * 0.001

snowyTerrain = false

opacityController = 0
opacityGoal = 55

playJumpSounds = true
handGrowthOnJump = true

speedParticleTimer = 0

gPaletteSupported = true

gSDConfig = {
    squashAndStretch = true,
    healthEffects = true,
    deathEffects = true,
    actionSounds = true,
    paletteEffects = true,
    particles = true,
    minorDetails = true,
}

local function sd_storage_key(key)
    return "sd_" .. key
end

local function sd_load_bool(key, defaultValue)
    local storageKey = sd_storage_key(key)
    if mod_storage_exists ~= nil and mod_storage_exists(storageKey) then
        return mod_storage_load_bool(storageKey)
    end
    return defaultValue
end

local function sd_save_bool(key, value)
    local storageKey = sd_storage_key(key)
    if mod_storage_save_bool ~= nil then
        mod_storage_save_bool(storageKey, value)
    end
end

gSDConfig.squashAndStretch = sd_load_bool("squash_and_stretch", gSDConfig.squashAndStretch)
gSDConfig.healthEffects = sd_load_bool("health_effects", gSDConfig.healthEffects)
gSDConfig.deathEffects = sd_load_bool("death_effects", gSDConfig.deathEffects)
gSDConfig.actionSounds = sd_load_bool("action_sounds", gSDConfig.actionSounds)
gSDConfig.paletteEffects = sd_load_bool("palette_effects", gSDConfig.paletteEffects)
gSDConfig.particles = sd_load_bool("particles", gSDConfig.particles)
gSDConfig.minorDetails = sd_load_bool("minor_details", gSDConfig.minorDetails)

playJumpSounds = gSDConfig.actionSounds
handGrowthOnJump = gSDConfig.minorDetails
gPaletteSupported = gSDConfig.paletteEffects

hook_mod_menu_text("Sharen's Details+ Settings")

hook_mod_menu_checkbox("Squash And Stretch", gSDConfig.squashAndStretch, function(_, value)
    gSDConfig.squashAndStretch = value
    sd_save_bool("squash_and_stretch", value)
end)

hook_mod_menu_checkbox("Health Effects", gSDConfig.healthEffects, function(_, value)
    gSDConfig.healthEffects = value
    sd_save_bool("health_effects", value)
end)

hook_mod_menu_checkbox("Death Effects", gSDConfig.deathEffects, function(_, value)
    gSDConfig.deathEffects = value
    sd_save_bool("death_effects", value)
end)

hook_mod_menu_checkbox("Action Sounds", gSDConfig.actionSounds, function(_, value)
    gSDConfig.actionSounds = value
    playJumpSounds = value
    sd_save_bool("action_sounds", value)
end)

hook_mod_menu_checkbox("Palette Effects", gSDConfig.paletteEffects, function(_, value)
    gSDConfig.paletteEffects = value
    gPaletteSupported = value
    sd_save_bool("palette_effects", value)
end)

hook_mod_menu_checkbox("Particles", gSDConfig.particles, function(_, value)
    gSDConfig.particles = value
    sd_save_bool("particles", value)
end)

hook_mod_menu_checkbox("Minor Details", gSDConfig.minorDetails, function(_, value)
    gSDConfig.minorDetails = value
    handGrowthOnJump = value
    sd_save_bool("minor_details", value)
end)

---------- Chat Commands ----------

function on_sns_v_air_command(msg)
    if tonumber(msg) then
        djui_chat_message_create("Set value to " .. msg)
        stretchVModifierAir = tonumber(msg) * 0.001
    else
        djui_chat_message_create("That's not a number!")
    end
    return true
end

function on_sns_v_ground_command(msg)
    if tonumber(msg) then
        djui_chat_message_create("Set value to " .. msg)
        stretchVModifierGround = tonumber(msg) * 0.001
    else
        djui_chat_message_create("That's not a number!")
    end
    return true
end

function on_sns_h_command(msg)
    if tonumber(msg) then
        djui_chat_message_create("Set value to " .. msg)
        stretchHModifier = tonumber(msg) * 0.001
    else
        djui_chat_message_create("That's not a number!")
    end
    return true
end

function on_jump_sound_command(msg)
    if msg == "on" then
        djui_chat_message_create("Jumping sounds are now on.")
        playJumpSounds = true
    elseif msg == "off" then
        djui_chat_message_create("Jumping sounds are now off.")
        playJumpSounds = false
    else
        return false
    end
    return true
end

function on_hand_growth_command(msg)
    if msg == "on" then
        djui_chat_message_create("Mario's right hand will now grow when jumping.")
        handGrowthOnJump = true
    elseif msg == "off" then
        djui_chat_message_create("Mario's right hand will no longer grow when jumping.")
        handGrowthOnJump = false
    else
        return false
    end
    return true
end

hook_chat_command("scale_v_air", "[number] - Sets how much Mario should stretch verticaly in the air, default value is 5.5", on_sns_v_air_command)
hook_chat_command("scale_v_ground", "[number] - Sets how much Mario should stretch verticaly in the ground, default value is 2.5", on_sns_v_ground_command)
hook_chat_command("scale_h", "[number] - Sets how much Mario should stretch horizontaly, default value is 2.8", on_sns_h_command)
hook_chat_command("jump_sound", "[on|off] - Enables or disables the sounds you make when you jump.", on_jump_sound_command)
hook_chat_command("hand_growth_jump", "[on|off] - Enables or disables Mario's hand growing in size when you jump.", on_hand_growth_command)

---------- Hooks ----------

--- @param m MarioState
function mario_update(m)
    local d = gMarioDetails[m.playerIndex]

    -- read this as:
    -- (m.area.terrainType & TERRAIN_MASK) == TERRAIN_SNOW ? true : false
    snowyTerrain = (m.area.terrainType & TERRAIN_MASK) == TERRAIN_SNOW and true or false

    if m.action == ACT_FROZEN then
        d.burnOrFreezeTimer = -60
    elseif snowyTerrain and ((m.action & ACT_GROUP_MASK) == ACT_GROUP_SUBMERGED) then
        d.burnOrFreezeTimer = approach_f32(d.burnOrFreezeTimer, -60, 2, 2)
    else
        d.burnOrFreezeTimer = approach_f32(d.burnOrFreezeTimer, 0, 1, 1)
    end

    if m.action == ACT_LAVA_BOOST then
        if m.health == 0xFF then
            set_mario_action(m, ACT_LAVA_DEATH, 0)
        end

        if d.grounded and m.floor.type == SURFACE_BURNING then
            if snowyTerrain then
                d.burnOrFreezeTimer = -60
            else
                d.burnOrFreezeTimer = 60
            end
        end
    end

    if gSDConfig.deathEffects then
        -- Anti-softlock (thank you sm64)
        if gDeathActions[m.action] and (m.action ~= ACT_LAVA_DEATH and m.action ~= ACT_FELL_OFF_STAGE and m.action ~= ACT_FROZEN) and not mario_can_bubble(m) then
            local savedVel = m.vel.y
            local savedPos = m.pos.y
            local savedFakePos = m.marioObj.header.gfx.pos.y

            if playSoundTimer >= DEATH_THEME_DURATION and m.action ~= ACT_DROWNING then
                common_death_handler(m, m.marioObj.header.gfx.animInfo.animID, m.marioObj.header.gfx.animInfo.animFrame)
                m.vel.y = savedVel
                m.pos.y = savedPos
                m.marioObj.header.gfx.pos.y = savedFakePos
                if m.action == ACT_QUICKSAND_DEATH then
                    if m.quicksandDepth > 170 then
                        m.quicksandDepth = 170
                    end
                end
            end

            m.marioBodyState.eyeState = MARIO_EYES_DEAD
        end

        if (m.action & ACT_GROUP_MASK) ~= ACT_GROUP_CUTSCENE and m.action ~= ACT_BUBBLED and m.floor ~= nil and
        (m.floor.type == SURFACE_DEATH_PLANE or m.floor.type == SURFACE_VERTICAL_WIND) and m.pos.y < m.floorHeight + 2048 then
            set_mario_action(m, ACT_FELL_OFF_STAGE, 0)
        end
    end

    if m.action ~= ACT_SQUISHED and not (m.action == ACT_LAVA_DEATH and snowyTerrain) then
        d.frameBeforeSquishOrFreeze = m.marioObj.header.gfx.animInfo.animFrame
    end

    if playJumpSounds and not d.playedSound then
        if m.action == ACT_DOUBLE_JUMP then
            run_audio(gSounds.SOUND_DOUBLE_JUMP, m, 1.55)

        elseif m.action == ACT_BACKFLIP or m.action == ACT_SIDE_FLIP then
            run_audio(gSounds.SOUND_FLIP_JUMP, m, 1.55)

        elseif m.action == ACT_TRIPLE_JUMP or m.action == ACT_SPECIAL_TRIPLE_JUMP or m.action == ACT_FLYING_TRIPLE_JUMP or
        (m.action == ACT_LONG_JUMP and m.marioObj.oMarioLongJumpIsSlow == 0) then
            run_audio(gSounds.SOUND_LONG_TRIPLE_JUMP, m, 1.55)
        end

        d.playedSound = true
    end

    if gSDConfig.squashAndStretch then
        squash_and_stretch(m)
    end

    if gSDConfig.particles then
        frame_perfect_chances(m)
        handle_dust_mario_update(m)
    end

    if gSDConfig.healthEffects then
        spawn_extra_health_fb(m)
    end

    if gPaletteSupported and gSDConfig.paletteEffects then
        toast_or_freeze_the_mario(m)
    end

    if gSDConfig.minorDetails then
        minor_details(m)
    end
end

function on_hud_render()
    if gSDConfig.deathEffects then
        too_bad()
    end
end

---@param m MarioState
function on_set_mario_action(m)
    local d = gMarioDetails[m.playerIndex]

    if gSDConfig.particles then
        handle_dust_on_set_mario_action(m)
    end

    if gSDConfig.actionSounds then
        if m.action == ACT_DOUBLE_JUMP or m.action == ACT_BACKFLIP or m.action == ACT_SIDE_FLIP or m.action == ACT_TRIPLE_JUMP or
        m.action == ACT_SPECIAL_TRIPLE_JUMP or m.action == ACT_FLYING_TRIPLE_JUMP or (m.action == ACT_LONG_JUMP and m.marioObj.oMarioLongJumpIsSlow == 0) then
            d.playedSound = false
        end
    end

    if m.action == ACT_SHOT_FROM_CANNON then
        if gSDConfig.particles then
            spawn_non_sync_object(id_bhvJumpDust, E_MODEL_EXPLOSION, m.pos.x + m.vel.x, m.pos.y + m.vel.y, m.pos.z + m.vel.z, nil)
        end
        d.burnOrFreezeTimer = 120
        d.lerpVariable = 1
        play_sound(SOUND_GENERAL2_BOBOMB_EXPLOSION, m.marioObj.header.gfx.cameraToObject)
        if m.playerIndex == 0 then
            set_environmental_camera_shake(SHAKE_ENV_BOWSER_JUMP)
        end
    end

    if m.action == ACT_BUBBLED then
        m.marioObj.header.gfx.node.flags = m.marioObj.header.gfx.node.flags | GRAPH_RENDER_ACTIVE
    end

    if gSDConfig.deathEffects then
        if m.action == ACT_DROWNING and snowyTerrain then
            set_mario_action(m, ACT_FROZEN, 0)
        end
    end

    if m.action == ACT_GROUND_POUND then
        m.marioObj.header.gfx.angle.x = 0
        m.marioObj.header.gfx.angle.y = m.faceAngle.y
        m.marioObj.header.gfx.angle.z = 0
    end
end

hook_event(HOOK_MARIO_UPDATE, mario_update)
hook_event(HOOK_ON_HUD_RENDER, on_hud_render)
hook_event(HOOK_ON_SET_MARIO_ACTION, on_set_mario_action)
hook_event(HOOK_BEFORE_MARIO_UPDATE, function(m) gMarioDetails[m.playerIndex].prevYVel = m.vel.y end)

hook_event(HOOK_ON_DEATH, function()
    if not gSDConfig.deathEffects then
        return
    end
    if playSoundTimer < DEATH_THEME_DURATION and not mario_can_bubble(gMarioStates[0]) then
        return false
    end
end)

hook_event(HOOK_UPDATE, function()
    speedParticleTimer = speedParticleTimer + 1
    if speedParticleTimer > 3 then
        speedParticleTimer = 0
    end

    if opacityController == opacityGoal then
        opacityGoal = -opacityGoal
    end

    opacityController = approach_f32(opacityController, opacityGoal, 5.6, 5.6)
end)