# LinkUp  
*A device that links communities and people togther*

Meet **Linki** — a small companion that lives in shared spaces and helps people connect more naturally.

---

## The Problem

LinkUp is designed to help bring social communities together. In a situation like moving into halls in the first year, socialising can be quite daunting. Group chats are filled with bin rotas and complaints.

Not because they don’t care, but because:
- initiating feels awkward or intrusive  
- messages get lost or ignored in group chats  
- there’s no signal that anyone actually *noticed* the invite  

Over time, this friction leads to silence — even when people genuinely want to connect.

---

##  The Solution

LinkUp allows the user to propose hangouts through our Linki device that react, accounce and reward social initiative. This allows the user to propose a hangout in a friendly way to break the ice.

Instead of opening a phone or typing a message, users interact with **Linki**, a friendly physical pet that:
- lets you propose a hangout with simple button presses  
- reacts emotionally when something happens  
- shares the invite in a shared physical space  
- gently rewards the act of reaching out  

---

##  Technology Used

ESP8266 (ESP-12E) × 2

OLED display (1.3" I2C)

Raspberry Pi

---

##  Backend

LinkUp uses a lightweight, local backend designed to be fast and reliable. Instead of relying on cloud services, the system runs a local HTTP-based backend within the shared space. Hangout proposals are sent as json files over the local network that span to an estimated radius of 200m, where they are immediately acknowledged and distributed to other Linkis. Powered by Arduino IDE and C++, the backend communicate messages instantly and all communication stays local and secured rather than being routed through third-party servers.


---

##  Frontend

React Native & Expo: Built for iOS and Android, focusing on a minimal but useful AI dashboard for the Linki.

Real Time Network: The devices track connectivity data that inputs to the backend to detect if the signal can be broadcast to a linked device on the network.

Integration of Google Gemini API: AI Agent provides an AI Overview on the Linki dashboard to give customized feedback to the user.

---

##  Challenges we ran into

Connecting the 2 devices together: Initially connecting the 2 devices was a challenging task. We had attempted to link them directly using board specific MAC addresses but instead found the solution of the devices creating their own network to connect to each other.

Wiring the hardware: Wiring the circuit together so that the buttons worked, and the screen was able to display our graphics was a challenge. Through trial and error and research into pin in and pin out functions we were able to complete a working

---

## What's next for LinkUp

LinkUp imagines a future where:
- social technology is ambient, not demanding  
- starting a hangout feels safe and encouraged  
- shared spaces actively support connection  

Considering developing more functions for Linki such as more faces, customisability and other animal shaped cases, introducing sound modules for pings and minimising size

---

**LinkUp**  
*Small device. Real connections.*

---
