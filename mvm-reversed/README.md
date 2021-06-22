## What is this
This is a project to reverse-engineer the MvM-related parts of Team Fortress 2. I go through the binaries, one function at a time, and attempt to turn the raw x86 assembly code into the C++ code that would have originally been used to program the game.

There is no intention for this project to be compileable, but I do intend for the source code to be fully valid. I'm using C++11 and C++14 features, because I can.

Things that are not related to MvM, especially including things that can be found in the public Source SDK, are omitted.

This project is currently based on the 20151007a revision of Team Fortress 2 (October 7, 2015 @ 00:35:00 UTC).

## Goals
* Better understanding of how the NextBot AI system works
* Precise information on fields and values allowed in pop files
* Making it easier to track down bugs in the game
* Making it easier to produce mods for the game

## Structure

### Population: pop file stuff
* Populators
  * RandomPlacement
  * PeriodicSpawn
  * Mission
  * WaveSpawn
  * Wave
* Spawners
  * Mob
  * SentryGun
  * Tank
  * TFBot
  * Squad
  * RandomChoice

### NextBot: generic AI system
* 

### CTFBot: Team Fortress specific AI system
* 

### Useful: useful resources for reverse engineering
* SDK-*: symlinks to Source SDK 2013 headers
* classgraph: dumps of the RTTI class hierarchy
* paths: string dumps of source file paths
* vtable: dumps of virtual function tables

## Progress

### Done
* IContextualQuery
* CKnownEntity
* IEventResponder
* Behavior<T>
* Action<T>
* IPopulationSpawner
* CMobSpawner
* CSentryGunSpawner
* CTankSpawner
* CSquadSpawner
* CRandomChoiceSpawner

### In progress
* CTFBotSpawner (90%)
* IVision (50%)
* CTFBot (1%)
* INextBotComponent
* NextBotPlayer<T>
* INextBot
* CMissionPopulator

### To do
* Other populators
* CPopulationManager
* IBody
* ILocomotion
* IIntention
* Path
* CTFBot actions

## Legality
Most of this is probably legal via fair use. I wouldn't bet my life on it though.
