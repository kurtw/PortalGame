# Summary
This is meant to be a Game Feature Plugin for the [Lyra Sample Game](https://dev.epicgames.com/documentation/en-us/unreal-engine/lyra-sample-game-in-unreal-engine) that adds networked portal mechanics in the third person perspective.

Developed on macOS Sonoma 14.2.1 with UE 5.3.2.
# Setup  
Clone the plugin and add it to the Game Features directory in a fresh install of Lyra or an existing project.\
**ProjectName > Plugins > GameFeatures** \
\
It is recommended to clean the build. i.e. In **Rider Build > Clean Solution**.\
  
<u>Project Settings</u>
- `Support global clip plane for Planar Reflections = true`. This is not strictly required, but enabling it allows for any surface to be used for the portals. Without it, custom planes and a specific level layout would be required so that the Capture Component can correctly see through the surface.  
  
<u>BP_Portal</u>  
- Set InitialPortalMaterial (`M_InitialPortalMaterial`)
  
<u>BP_PortalPlayerController</u>  
- Set portal class (`BP_Portal`)
  
<u>BP_PortalHero</u>  
- Change *CharacterMovement* component class to *PortalMovementComponent*  
  
<u>GA_Portal</u>  
- Set widget class (`WBP_Portal`), portal class (`BP_Portal`), portal effect class (`GE_PortalEffect`)  
- Set ability tags *Ability.Type.Action.Portal*  
- Set ability triggers for *Event.PortalA* & *Event.PortalB*
## Play
After cloning the plugin and adding it to a project, **open L_TestMap > use PIE**.
### Controls
- **LMB**: spawn portal A
- **Shift + LMB**: destroy portal A
- **RMB**: spawn portal B
- **Shift + LMB**: destroy portal A

All other controls are the same as the Elimination experience.
# Description of Changes
## C++
<u>Portal</u>  
- The main class for the portal. Handles the portal rendering and teleportation logic. Currently `TeleportActor` just handles teleporting players, but can be extended to teleport objects.  
  
<u>PortalPlayerController</u>  
- Handles portal spawn logic, but uses `SpawnActorDeferred` to allow the ability to pack `EffectContextHandle` in PortalAbility with data to pass around.  
  
<u>PortalAbility</u>  
- Handles ability activation and removal. Inside `FinishSpawningPortal`, `FinishSpawning` is called to complete `SpawnActorDeferred` from the controller.  
  
- There was an issue with the UIExtension not correctly unregistering in blueprint. The issue was resolved by adding a one tick delay to garbage collection. There is a note about it in the code. In short, an ability instance is marked for garbage collection before blueprint function `K2_OnAbilityRemoved` is called, so this results in the blueprint event `OnAbilityRemoved` not properly working.  
  
<u>PortalAttributeSet</u> 
- This adds the attribute *MovementSpeed* that is used for the player speed boost after teleportation.  
  
<u>PortalMovementComponent</u>  
- Created to support the *MovementSpeed* attribute from PortalAttributeSet.  
## Blueprint
<u>BP_Portal</u>  
- Handles the invisible effect after teleporting. The effect is only applied to the hero avatar. For the weapon, I chose to use the same method Fortnite uses by setting the relative scale to 0 and ramping it back up to 1.  
  
<u>BP_PortalPlayerController</u>  
- Handles input for creating and destroying portals. `SendGameplayEventWithPayload` is called to activate the portal ability.  
- The distance a portal can be spawned from the player can be adjusted using *PortalDistanceModifier*.  
  
<u>BP_PortalHero</u>  
- `LinkAnimLayers M-F` was created to resolve an issue with Lyra when starting a game unarmed. This function will correctly link the matching animations for the male, female avatars.  
- The CharacterMovement component classes uses *PortalMovementComponent* instead of *LyraCharaterMovementComponent* to apply a speed boost after teleporting.  
  
<u>M_PortalMannequin</u>  
- Uses *PortalMaterialFade* parameter to apply `DitherTemporalAA` for the invisible effect after teleporting.  
  
<u>GA_Portal</u>  
- Handles registering and unregistering the portal widget as well as adding and removing the input mapping for the portal controls (`IMC_Portal`). When the ability is added, the portal widget is registered the UI will update the quick bar and HUD to show the portal icons.  
- Handles ending the ability by listening for changes of the attribute *PortalAttributeSet.MovementSpeed*.  
  
<u>GE_PortalEffect</u>  
- Applies a movement speed boost for 3 seconds.  
- Blocks the dash ability (*Ability.Type.Action.Dash*) while the speed boost is active.  
  
<u>WBP_PortalHUDLayout</u>  
- Holds the new slot for ExtensionPoint *HUD.Slot.Ability*, which is the location of `WBP_Portal`.  
  
<u>WBP_Portal</u>  
- Handles the portal status indicator on the HUD. When a portal is active a corresponding color is shown. Blue for Portal A and Orange for Portal B. This widget also shows the control input for spawning a portal, left and right mouse button in this case.  
  
<u>ID_PortalItem</u>  
- This is an inventory fragment that uses equipment definition `WID_Portal` to grant `AbilitySet_PortalAbility` and attach `BP_PortalItem` to *weapon_r* socket. `BP_WeaponInstance_PortalItem` is set, but only to use the Animations for the item. Portal mesh was created to use in `BP_PortalItem`.  
  
<u>BP_GrantAbility</u>  
- This was made with inspiration from `B_WeaponSpawner`, but instead it grants and ability and adds a corresponding item to the quick bar. When the player overlaps this actor, they are given a portal item, which adds `AbilitySet_PortalAbility` to the player's Ability System Component. `AbilitySet_PortalAbility` grants `GA_Portal` and `PortalAttributeSet`.  
  
<u>BP_RemoveAbility</u>  
- Handles removal of the portal item and input mapping `IMC_Portal`.  
  
<u>Experiences and Game directories</u>  
- Blueprints and data assets copied over and customized to use in the new map `L_TestMap`.  
- `BP_PortalExperience` is used to set all relevant settings and data. This experience is tied to the playlist `DA_PortalPlaylist` so that it can be access through the `LyraFrontEnd`.  
- `DA_PortalTagRelationships` is added, but unchanged from default for now.
# Known Limitations and Future Work
Bugs and Enhancements are outlined here: https://github.com/kurtw/PortalGame/issues
