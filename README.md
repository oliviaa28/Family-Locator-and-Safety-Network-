# 📍 Family Locator & Safety Network ✨

Client–server application designed to improve family safety and coordination through **real-time location sharing and emergency alerts**.

The system allows family members to stay connected, track locations and receive instant notifications in case of emergencies.

---

## ⚙️ Technologies

- **C** – core implementation
- **TCP sockets** – reliable client–server communication
- **select()** – server-side concurrency for multiple clients
- **SQL database** – persistent storage of users and families
- **raylib + Dear ImGui** – graphical user interface

---

## 🏗 Architecture ｡𖦹°‧

The system follows a **centralized client–server architecture**.

- The **server** manages connections, validates requests and synchronizes data between users.
- The **clients** handle only the graphical interface and send commands to the server.

All important data such as:

- user accounts
- family groups
- location history

is stored in a **relational SQL database** to ensure persistence.

---

##  ⋆｡‧˚ʚ Featuresɞ˚‧｡⋆

- user **registration and authentication**
- creation and management of **family groups**
- **real-time location sharing**
- **SOS emergency alerts**
- **multiple notification types**
- automatic alerts when entering or leaving predefined locations
- ability to belong to **multiple families**
- graphical interface for easy interaction


---

## 🖥 Interface

The graphical interface is implemented using **raylib and Dear ImGui**, providing an intuitive interaction model.

Main components include:

- login and registration window
- family management interface
- real-time location display
- settings and notifications panel
## 🖼 Interface

<p align="center">
  <img src="https://github.com/user-attachments/assets/139201a2-046a-4f8b-b719-f7d860c8df05" width="800"/>
</p>

<p align="center">
  <img src="https://github.com/user-attachments/assets/d5ca44a6-1cd4-4a5d-8d22-655940aadce8" width="800"/>
</p>
---

## 📊 Project Purpose

This project focuses on understanding and implementing:

- **network programming using TCP sockets**
- **handling multiple clients with select()**
- **client–server architecture**
- **database integration**
- **GUI development**

---
