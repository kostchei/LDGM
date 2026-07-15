# Legally Distinct Gorka Morka (LDGM) — Audio Design & Development Guide

This document establishes the audio design framework, research findings, and technical implementation plan for **Legally Distinct Gorka Morka (LDGM)**. 

Audio in LDGM is not merely aesthetic; it is a primary telemetry feedback channel. Because the game uses a first-person cockpit camera and explicitly excludes runtime panel deformation or detailed live damage artwork, the player relies on mechanical, acoustic, and environmental audio to judge their vehicle’s condition, terrain traction, speed, and combat status.

---

## 1. Core Vehicle Audio Design

To sell the promise of driving a **heavy, imperfect, improvised machine** (rather than a free-floating camera), the vehicle soundscape must be tactile, dynamic, and physically responsive.

### 1.1 Engine RPM & Load Modulation
A static, pitch-shifted loop sounds artificial. The engine must respond to two primary parameters from the physics engine (Project Chrono):
*   **RPM (Revolutions Per Minute):** Maps to the fundamental pitch and cycle rate of the engine.
*   **Load (Throttle / Torque Demand):** Differentiates between the engine working under strain (accelerating, climbing sand dunes) and coasting (foot off the throttle, descending).

#### Implementation Strategy:
*   **Layered Blend Containers:** Utilize audio middleware (e.g., Wwise or FMOD via O3DE ATL) to map two independent axes: RPM ($x$-axis) and Load ($y$-axis).
*   **On-Load vs. Off-Load Samples:** Use separate sample sets:
    *   *On-Load:* Throaty, aggressive, high-pressure combustion sounds, intake growl, exhaust roar.
    *   *Off-Load:* Low-pressure coasting, transmission drag, engine braking whine, mechanical chatter.
*   **Crossfading & Pitch-Shifting:** Blend between loops recorded at specific RPM intervals (e.g., every 1000 RPM) rather than stretching a single loop too far. This avoids the "chipmunk effect."

```
        ▲ High |   On-Load Layer (Aggressive combustion, intake roar)
        │      |   * Dynamic crossfading between 1k, 2k, 3k, 4k, 5k RPM
  LOAD  │      |   
  (RTPC)├──────┼────────────────────────────────────────────────────────
        │      |   Off-Load Layer (Mechanical rattle, transmission drag)
        │ Low  |   * Muted combustion, gear whine, engine braking
        └──────┴────────────────────────────────────────────────────────
               Low                       RPM (RTPC)                       High ▶
```

### 1.2 Multi-Perspective Balancing (Exhaust, Intake, Block)
Because the player is locked in a first-person cockpit camera, the dry, direct engine sounds must feel like they are coming from the correct physical locations:
*   **Engine Block (Front/Chassis):** Low-to-mid frequency vibrations, timing chain rattle, valve clicks. Transmitted structurally through the dashboard and bulkhead.
*   **Intake (Cabin/Bulkhead):** Sucking air, throttle body hiss, and throatiness under high throttle.
*   **Exhaust (Rear/Tailpipe):** Low-frequency rumble, backfires, popping on deceleration. Attenuated heavily by the cabin bulkhead but still audible as a low-end thump.

### 1.3 Cockpit Acoustic Filtering
The cockpit cabin acts as a physical barrier.
*   **High-Frequency Attenuation:** The metal shell and plexiglass/mesh windshield filter out high frequencies. Apply a permanent low-pass filter (LPF) to external sounds (wind, distant gunfire, gravel hits).
*   **Low-Frequency Resonance:** The cabin acts as a resonant chamber. Bass frequencies (engine rumble, heavy suspension thuds, scraping) should resonate around 80Hz - 150Hz.
*   **Mechanical Rattles:** Introduce cockpit-bound rattling layers (loose screws, shaking dashboards, cabin wires hitting paneling) that trigger and scale their volume based on chassis vibration and road roughness telemetry.

---

## 2. Weapon Sound Design (Guns & Rockets)

Weapons in LDGM are heavy, primitive, and destructive. The sound design must convey their sheer kinetic energy while respecting the cockpit boundary.

### 2.1 Gunfire (Autocannons, Heavy Machine Guns)
*   **The Muzzle Blast (External):** A sharp, high-pressure transient that tears the air. This layer is heavily low-pass filtered when heard from inside the cabin.
*   **The Chassis Thud (Internal):** A massive, low-frequency structural impulse. When a hull-mounted gun fires, the entire vehicle frame vibrates. We simulate this with a dry, punchy $100\text{Hz}$ thump that bypasses the cabin attenuation, making the player feel the recoil.
*   **Shell Casing Clatter:** As shells eject, we trigger randomized metal clatter pools. Since they eject onto the flatbed or metal chassis plates, they should sound like heavy brass hitting rusted steel panels close to the cabin.
*   **Mechanical Reload/Cycles:** Distinct, mechanical clinks, clanks, and air-pressure releases for weapon reloading, giving an industrial feel.

### 2.2 Rockets & Missiles
*   **Launch Ignition:** Avoid a clean "whoosh." Use a sudden, high-pressure sizzle, followed by a low-frequency, bassy rocket motor ignition roar. The initial blast should apply a temporary ducking effect to the cabin engine sound.
*   **In-Flight Trail:** A tearing, whistling hiss that fades out rapidly as it accelerates away. 
*   **Detonation & Thunder:** Explosion impacts should feature a sharp metal-tearing transient, followed by a heavy low-end blast. 

### 2.3 Acoustic Echo & Propagation
To sell the scale of the environment (canyons, ruins, open wastes), high-amplitude sounds (gunfire, rocket explosions) must propagate through the world:
*   **Speed of Sound Delay:** Calculate the distance ($d$) from the explosion to the player cockpit. Delay the audio playback by $t = d / 343$ seconds. Hearing a rocket hit a distant ridge, followed by a delayed thunderclap $1.5\text{ seconds}$ later, dramatically increases the sense of scale.
*   **Canyon Reflections:** Trigger a secondary slap-back echo delay line ($250\text{ms}$ - $600\text{ms}$) with high feedback and low-pass filtering when weapons fire in valleys or canyons.

---

## 3. Surface & Interaction Acoustics

Tire-to-ground interactions tell the player about traction, soil density, and wheel slip.

```
                  ┌─────────────────────────────────────────┐
                  │          Tire/Surface Contact           │
                  └────────────────────┬────────────────────┘
                                       │
                ┌──────────────────────┴──────────────────────┐
                ▼                                             ▼
     Rolling Texture (Continuous)                  Impact/Slip (Transient)
  ┌─────────────────────────────────┐           ┌──────────────────────────────────┐
  │ • Sand: Soft whispering hiss    │           │ • Sand: Heavy churning/spinning  │
  │ • Gravel: Crunchy low-end rumble│           │ • Gravel: High-freq stone pelt   │
  │ • Roadkill: Sticky wet rolling  │           │ • Roadkill: Squash/Thud/Splat    │
  └─────────────────────────────────┘           └──────────────────────────────────┘
```

### 3.1 Sand (Firm Sand to Deep Sand)
*   **Rolling Texture:** A soft, whispering white-noise hiss. There should be little to no sharp transient detail.
*   **Deep Sand Drag:** As wheels dig in, fade in a low-frequency churning, rushing sound that indicates engine strain and wheel drag.
*   **Undercarriage Spray:** A soft, continuous pelting sound (like sugar falling on cardboard) representing sand thrown up against the wheel arches.

### 3.2 Gravel (Broken Road to Loose Rocks)
*   **Rolling Texture:** A dynamic, crunching low-end rumble mixed with continuous grit.
*   **Undercarriage Pelting:** High-frequency, sharp impact sounds (dinks and tacks) representing individual stones striking the metal wheel arches and floor pan.
    *   *Foley Source:* Raw dried pasta (macaroni/conchiglie) dropped onto sheet metal or dry beans shaken in a metal tin.
    *   *Dynamic Scaling:* Scale the frequency and volume of these impact sounds directly with wheel rotational speed and slip ratio.
*   **Skidding/Sliding:** A dry, scraping crunch that blends from the rolling texture when wheels lose traction.

### 3.3 Roadkill under Wheels
Vehicular combat leaves debris and biological hazards on the road. Running over a biological entity must feel visceral and heavy:
*   **The Initial Bump (Thud):** A heavy, low-frequency suspension thump. The chassis lifts and settles, conveying the mass of the vehicle rolling over a substantial object.
*   **The Visceral Crunch:** A wet, crisp snap representing bones and exoskeleton breaking under high tonnage.
    *   *Foley Source:* Fresh celery stalks snapped sharply near a microphone, layered with crushing dry walnuts or cabbage.
*   **The Wet Splat (Squelch):** A mid-frequency, wet squelch as organic material is compressed and ejected.
    *   *Foley Source:* Crushing ripe melons, grapefruit, or squeezing wet towels soaked in soapy water/mayonnaise.
*   **Undercarriage Splat:** A brief, wet spray layer played on the rear cabin wall, representing debris kicked up by the tires.

---

## 4. Physics-to-Audio Integration (Chrono Telemetry)

To bridge Project Chrono's physics simulation with O3DE's audio system, we translate force and velocity vectors into real-time audio parameters.

```
       [ Project Chrono Physics ]
                   │
                   ▼
     (Expose Telemetry over Adapter)
                   │
  ┌────────────────┼────────────────┐
  ▼                ▼                ▼
[Normal Force]  [Friction Force]  [Slip Velocity]
  │                │                │
  ▼                ▼                ▼
[Impact Thuds]  [Grinding Loop]  [Surface Skids]
  │                │                │
  └────────────────┼────────────────┘
                   ▼
       [ O3DE ATL (Wwise/FMOD) ]
```

### 4.1 Impact Thuds (Chassis Collisions)
When the vehicle hits a boulder, wall, or another vehicle, the impact audio must match the physical force.
*   **Data Extraction:** Read the normal contact force vector ($\vec{F}_N$) and the relative impact velocity ($\vec{v}_{rel}$) from Chrono's contact points.
*   **Audio Mapping:** 
    *   *Low Impulse:* A metallic rattle and quick spring squeak.
    *   *Medium Impulse:* A solid, hollow metal thud layered with bumper buckling.
    *   *High Impulse:* A massive, low-frequency slam, cabin metal groans, glass rattling, and temporary cockpit audio ducking (simulating ear ringing/pressure).
*   **RTPC Modulation:** Map impact magnitude to the audio event's volume and low-pass filter frequency (harder hits have more high-frequency metallic bite).

### 4.2 Chassis Grinding & Scraping
If a vehicle scrapes against a concrete barrier, rock wall, or another chassis:
*   **Data Extraction:** Read the friction force vector ($\vec{F}_f$) and the relative sliding velocity ($\vec{v}_{slide}$) along the contact surface.
*   **Audio Loop:** Trigger a continuous grinding sound loop (metal-on-metal or metal-on-stone).
*   **Parameter Linking:**
    *   *Volume:* Linked to the friction force ($\vec{F}_f$) — harder scrapes are louder.
    *   *Pitch & Texture:* Linked to sliding velocity ($\vec{v}_{slide}$) — faster scrapes are higher pitched and feature more screeching transients; slow scrapes are deep and grumbly.

### 4.3 Suspension Bottoming Out
First-person driving feels weightless unless the suspension limits are audible.
*   **Data Extraction:** Monitor the suspension displacement ($z_{def}$) and velocity ($\dot{z}_{def}$) for each wheel in Chrono.
*   **Trigger Condition:** If $z_{def} \ge z_{max}$ (suspension travel fully compressed):
    *   Trigger a sharp, metallic slam representing the bump-stop hitting the chassis.
    *   Play a loud rebound spring groan on release.
*   **Spatialization:** Spatialize the thuds to the four corners of the cockpit (Front-Left, Front-Right, Rear-Left, Rear-Right) based on which wheel bottomed out.

---

## 5. Development & Tranche Implementation Plan

To ensure audio keeps pace with physical implementation, we align sound assets with the project's tranches:

### Tranche T1: The Driving Foundation
*   **Goals:** Basic engine, cabin acoustics, and surface sounds.
*   **Assets:**
    *   Starter technical engine loop (RPM-only).
    *   Basic sand, gravel, and asphalt rolling loops.
    *   Cockpit low-pass filter profile.
*   **Chrono Bridge:** Expose wheel speed and basic RPM/Throttle to O3DE ATL.

### Tranche T2: Combat and Impact Feedback
*   **Goals:** Gunfire, rockets, collisions, and damage indicators.
*   **Assets:**
    *   Chassis thuds and muzzle blasts for starter weapons.
    *   Rocket ignition, trail, and explosion.
    *   Dynamic impact thuds (low, med, high).
    *   Grinding loop (metal-on-stone, metal-on-metal).
    *   Damaged state audio (mechanical knocks, slipping clutch, blowing manifold hiss).
*   **Chrono Bridge:** Expose collision contact force, friction force, relative sliding velocity, and suspension bottoming states.

### Tranche T3: Environmental Polish & Vertical Slice
*   **Goals:** Surface grit, roadkill, and echoes.
*   **Assets:**
    *   Undercarriage stone pelting (frequency scales with speed/slip).
    *   Roadkill crunch & squish.
    *   Canyon reverb zones and speed of sound delay lines.
