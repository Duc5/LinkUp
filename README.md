🐾 LinkUp

A playful, physical way to spark community hangouts
LinkUp is a small, expressive social device that lives in a shared space (e.g. a student flat).
Instead of noisy group chats or awkward messages, residents propose hangouts through our Linki that react, announce, and reward social initiatives.

🌱 The Problem

In shared living spaces:

People hesitate to initiate hangouts

Messages get buried in group chats

There’s no feedback that anyone saw your invite

Social effort often goes unrewarded

This creates friction — even when people want to hang out.

💡 The Idea

Turn social initiation into a low-pressure, playful action.

HangoutPet:

lets users propose hangouts with one or two buttons

reacts with expressive eyes

broadcasts the invite locally over Wi-Fi

rewards social initiative with a visible XP system

optionally escalates the invite via a Raspberry Pi “Host Bot”

No phones. No apps. Just ambient social computing.

🧩 System Overview
Hardware

ESP8266 (ESP-12E) × 2

ESP A (Hub / Receiver)

creates local Wi-Fi network

receives events

displays reactions + event info

ESP B (Sender)

user interface

buttons + OLED

sends hangout proposals

OLED display (1.3" I2C)

Buttons (2)

Raspberry Pi (optional extension)

(Optional) robotic arm connected to Raspberry Pi

🖥️ Software Architecture
ESP A — Hub / Receiver

Creates Wi-Fi AP (HangoutNet)

Runs HTTP server (POST /event)

Default state: animated eyes

On event:

😲 Surprised eyes

📢 Event details (8s)

🙂 Return to idle eyes

Fully non-blocking (no delay() in networking path)

ESP B — Sender / Controller

Joins HangoutNet

Button-based UI:

Set time

Choose location

Confirm event

Sends JSON via HTTP POST

Gamified XP system

Each sent event increases Social XP

XP visualised as a progress bar

XP screen shown via long-press

Raspberry Pi (Extension)

Acts as a Host / Concierge AI

Receives events

Uses an AI agent to:

generate friendly invite messages

decide tone / suggestion

Controls a physical arm to “announce” hangouts

Can acknowledge events back to ESP devices

🎮 Gamification Layer

Social XP Bar (0–100%)

Earn XP by proposing hangouts

XP displayed locally on the device

Makes social effort visible and rewarding

Designed to motivate initiative, not spam

👁️ Expressive Design

The device behaves like a character:

Happy / Curious idle states

Blinking animation

Surprised reaction on incoming events

Emotional feedback makes the system feel alive

📡 Communication Protocol

POST /event

{
  "location": "Kitchen",
  "time_offset_h": 2,
  "sender_id": "Alex"
}


Local only (no cloud)

Fast ACK to avoid timeouts

Designed for multi-device scaling

🧪 Demo Flow (What Judges See)

Device sits idle, blinking

User proposes hangout on sender

XP bar increases 🎉

Receiver reacts with surprise

Event details appear

(Optional) Raspberry Pi arm announces invite

Device returns to calm idle state

🚀 Why This Wins

Community-focused, not productivity theater

Physical + digital + emotional feedback loop

No phone dependency

Scales naturally to more devices

Clear AI role (not bolted-on)

Delightful demo presence

🔮 Future Extensions

RSVP buttons on multiple senders

Mood-of-the-flat detection

Time-based reminders

Personalized AI host personalities

Persistent XP levels per resident

🛠️ Setup (Quick)

Flash ESP A (Hub) → creates HangoutNet

Flash ESP B (Sender) → joins network

Power both devices

(Optional) Run Raspberry Pi Flask server

Start proposing hangouts ✨

👥 Team

Built collaboratively during a hackathon, combining:

Embedded systems

Networked devices

Expressive UI

Human-centred design

AI agents

HangoutPet — because starting a hangout should feel fun, not awkward 🐾
