"""All lootable things in Gloshaugen Strike, in one place.

This file is the single source of truth:
  * Tools/Blender/gen_models.py builds a 3D model for every entry (SourceArt/Models/SM_<Id>.fbx)
  * Tools/ItemCatalog/gen_cpp.py writes Source/Gloshaugen/Private/GSItemCatalog.gen.inl
Edit here, then run:  python Tools/ItemCatalog/gen_cpp.py
Units: damage in HP (players have 100), distances in meters, times in seconds.
"""

RARITIES = ["Common", "Uncommon", "Rare", "Epic", "Legendary", "Mythic"]

# ------------------------------------------------------------------ effects (boosts and side effects)
# Modifier keys (1.0 = normal): speed, stamina_drain, stamina_regen, recoil, spread, damage, taken (damage taken),
# jump, reload (reload time), hunger, thirst (how fast they fall).
# Per second: regen (HP/s), drain (HP/s). Flags: nightvision, wallhack, phoenix, silent, flashimmune, ghost, glide.
# lifesteal = fraction of damage dealt you get back as health. visual: 0 none, 1 blur, 2 colors, 3 shake,
# 4 green haze, 5 psychedelic, 6 bright/bloom, 7 night vision.
EFFECTS = {
    "Turbo":        dict(name="Turbo", dur=40, speed=1.25, stamina_drain=0.0, reload=0.8, visual=3, after="TurboKrasj"),
    "TurboKrasj":   dict(name="Turbo-krasj", dur=30, speed=0.85, spread=1.5, stamina_regen=0.5, visual=1),
    "Gronn":        dict(name="Grønn", dur=60, speed=0.92, spread=0.6, recoil=0.7, taken=0.9, regen=0.3, hunger=2.5, visual=4),
    "Snus":         dict(name="Snus", dur=120, reload=0.9, spread=0.9),
    "Adrenalin":    dict(name="Adrenalin", dur=15, speed=1.15, taken=0.8, visual=6, after="AdrenalinKrasj"),
    "AdrenalinKrasj": dict(name="Adrenalin-krasj", dur=15, stamina_drain=2.0, stamina_regen=0.5),
    "Morfin":       dict(name="Morfin", dur=30, taken=0.65, speed=0.95, visual=1),
    "Bulk":         dict(name="Bulk", dur=90, damage=1.12, stamina_drain=0.8, after="BulkKrasj"),
    "BulkKrasj":    dict(name="Muskelkrampe", dur=30, spread=1.25),
    "Fokus":        dict(name="Fokus", dur=60, recoil=0.6, spread=0.7),
    "Nattoye":      dict(name="Nattøye", dur=90, nightvision=1, visual=7, after="Lysomfintlig"),
    "Lysomfintlig": dict(name="Lysømfintlig", dur=20, visual=6),
    "Lynet":        dict(name="Lynet", dur=30, jump=1.5, speed=1.1),
    "Iskald":       dict(name="Iskald", dur=45, silent=1, after="Skjelving"),
    "Skjelving":    dict(name="Skjelving", dur=20, spread=1.4, visual=3),
    "Berserk":      dict(name="Berserk", dur=30, damage=1.3, speed=1.1, drain=1.0, visual=2),
    "Spokelse":     dict(name="Spøkelse", dur=20, ghost=1, silent=1, visual=1),
    "Pansertablett": dict(name="Panser", dur=60, taken=0.75),
    "Regenerator":  dict(name="Regenerator", dur=30, regen=3.0, hunger=3.0),
    "Drom":         dict(name="Drøm", dur=40, flashimmune=1, taken=0.9, speed=0.9, visual=1),
    "Oyet":         dict(name="Øyet", dur=12, wallhack=1, visual=5, after="Forvirret"),
    "Forvirret":    dict(name="Forvirret", dur=15, spread=1.5, visual=2),
    "Nikotin":      dict(name="Nikotin", dur=180, stamina_drain=0.85, stamina_regen=1.3),
    "Foniks":       dict(name="Føniks", dur=120, phoenix=1, visual=6),
    "Blodtorst":    dict(name="Blodtørst", dur=45, lifesteal=0.25, visual=2),
    "Lattergass":   dict(name="Lattergass", dur=25, speed=1.2, spread=1.3, visual=2),
    "Koffein":      dict(name="Koffein", dur=90, stamina_drain=0.85),
    "KoffeinPlus":  dict(name="Koffein+", dur=60, stamina_drain=0.7, speed=1.05, reload=0.95),
    "Sukkerrush":   dict(name="Sukkerrush", dur=40, stamina_regen=1.5),
    "Matforgiftning": dict(name="Matforgiftning", dur=20, drain=1.0, speed=0.9, visual=1),
    "Smertestillende": dict(name="Smertestillende", dur=25, regen=1.0),
    "Kirurgi":      dict(name="Feltkirurgi", dur=20, regen=2.0),
    "Glide":        dict(name="Fallskjerm", dur=60, glide=1),
    "Avslort":      dict(name="Avslørt av drone", dur=10, wallhack=1),
    "Blendet":      dict(name="Blendet", dur=4, visual=6, spread=2.0),
    "Brennende":    dict(name="Brennende", dur=3, drain=6.0, visual=6),
    "Tran":         dict(name="Tran", dur=120, regen=0.4, stamina_regen=1.2),
    "Julestemning": dict(name="Julestemning", dur=60, taken=0.92, stamina_regen=1.2, visual=6),
    "Mett":         dict(name="Mett og god", dur=120, hunger=0.5, speed=0.97),
    "Sjokk":        dict(name="Elektrisk sjokk", dur=3, speed=0.3, spread=3.0, visual=3),
}

ITEMS = []


def _add(cat, id, name, rarity, desc="", **kw):
    assert rarity in RARITIES, rarity
    assert all(i["id"] != id for i in ITEMS), id
    d = dict(cat=cat, id=id, name=name, rarity=rarity, desc=desc)
    d.update(kw)
    ITEMS.append(d)
    return d


# ------------------------------------------------------------------ weapons
# cls: Pistol, SMG, Rifle, Sniper, Shotgun, LMG, Launcher, Bow, Melee
# mode: hitscan, pellets, projectile, melee
def gun(id, name, rarity, cls, dmg, interval, auto, mag, reserve, reload, rng, hip, aim, recoil, fov,
        model, desc="", **kw):
    return _add("Weapon", id, name, rarity, desc, cls=cls, mode=kw.pop("mode", "hitscan"), dmg=dmg, interval=interval,
                auto=auto, mag=mag, reserve=reserve, max_reserve=kw.pop("max_reserve", reserve * 3), reload=reload,
                range=rng, hip=hip, aim=aim, recoil=recoil, fov=fov, model=model, **kw)


def melee(id, name, rarity, dmg, interval, reach, model, desc="", **kw):
    return _add("Weapon", id, name, rarity, desc, cls="Melee", mode="melee", dmg=dmg, interval=interval, auto=True,
                mag=0, reserve=0, max_reserve=0, reload=0, range=reach, hip=0, aim=0, recoil=0.4, fov=85,
                model=model, headshot=1.5, **kw)


# --- Pistols
gun("M1911", "M1911 (Kongsberg Colt)", "Common", "Pistol", 28, 0.18, False, 7, 21, 1.6, 80, 1.6, 0.35, 1.4, 75,
    dict(custom="SM_Pistol_M1911", muzzle=0.17, sight=0.045), "Norsk tjenestepistol i mange tiår. Startvåpen.")
gun("Glock17", "Glock 17", "Common", "Pistol", 22, 0.12, False, 17, 34, 1.4, 70, 1.8, 0.4, 1.1, 75,
    dict(arch="pistol", slide=0.19, h=0.035, frame="poly", slidecol="dark"))
gun("Makarov", "Makarov PM", "Common", "Pistol", 20, 0.14, False, 8, 24, 1.4, 60, 1.8, 0.45, 1.2, 75,
    dict(arch="pistol", slide=0.16, h=0.03, frame="bake", slidecol="steel"))
gun("Luger", "Luger P08", "Uncommon", "Pistol", 26, 0.16, False, 8, 24, 1.6, 80, 1.6, 0.35, 1.3, 75,
    dict(custom="SM_Luger", muzzle=0.15, sight=0.032))
gun("Revolver", ".357 Revolver", "Rare", "Pistol", 46, 0.35, False, 6, 18, 2.6, 90, 1.8, 0.3, 3.2, 72,
    dict(arch="revolver", barrel=0.15), headshot=2.2)
gun("DesertEagle", "Desert Eagle", "Epic", "Pistol", 56, 0.3, False, 7, 14, 2.0, 100, 2.2, 0.35, 4.2, 72,
    dict(arch="pistol", slide=0.27, h=0.045, frame="steel", slidecol="chrome"), headshot=2.2)
gun("PistolKnife", "Pistolkniv", "Rare", "Pistol", 30, 0.25, False, 4, 12, 2.4, 40, 2.0, 0.6, 1.5, 78,
    dict(arch="pistolknife"), "Kniv og pistol i ett. Slag med V gjør ekstra mye skade.", melee_dmg=70)
# --- SMGs
gun("MP40", "MP40", "Uncommon", "SMG", 20, 0.11, True, 32, 64, 2.3, 60, 2.6, 0.7, 0.7, 70,
    dict(custom="SM_MP40", muzzle=0.34, sight=0.035))
gun("Uzi", "Uzi", "Common", "SMG", 18, 0.085, True, 25, 75, 2.0, 50, 3.0, 0.9, 0.7, 72,
    dict(custom="SM_Uzi", muzzle=0.26, sight=0.07))
gun("Suomi", "Suomi KP/-31", "Rare", "SMG", 19, 0.07, True, 70, 70, 3.4, 60, 2.6, 0.7, 0.6, 70,
    dict(custom="SM_Suomi", muzzle=0.5, sight=0.04), "Finsk trommemagasin med 70 skudd.")
gun("MP5", "MP5", "Rare", "SMG", 21, 0.075, True, 30, 90, 2.2, 70, 2.4, 0.5, 0.6, 68,
    dict(arch="rifle", rx=(-0.08, 0.17), rh=0.06, barrel=0.07, hg=(0.12, "poly", "round"), stock="retract", mag="curved_s",
         magx=0.08, sight="drum", furn="poly", muzzle="none"))
gun("Thompson", "Thompson M1928", "Epic", "SMG", 24, 0.085, True, 50, 100, 3.0, 65, 2.6, 0.6, 0.8, 68,
    dict(arch="rifle", rx=(-0.06, 0.2), rh=0.06, barrel=0.2, hg=(0.0, "wood", "none"), foregrip=True, stock="wood",
         mag="drum", magx=0.1, sight="iron", furn="wood", muzzle="comp", fins=True))
gun("PPSh", "PPSh-41", "Rare", "SMG", 18, 0.06, True, 71, 71, 3.4, 55, 3.0, 0.8, 0.6, 70,
    dict(arch="rifle", rx=(-0.05, 0.18), rh=0.055, barrel=0.18, hg=(0.18, "steel", "jacket"), stock="wood", mag="drum",
         magx=0.09, sight="iron", furn="wood", muzzle="none", wrist=True))
# --- Rifles
gun("AG3", "AG3", "Rare", "Rifle", 38, 0.1, True, 20, 60, 2.4, 300, 3.0, 0.25, 1.1, 55,
    dict(custom="SM_AG3", muzzle=0.9, sight=0.065), "Forsvarets gamle trofaste. Slår hardt.")
gun("AK47", "AK-47", "Rare", "Rifle", 34, 0.1, True, 30, 90, 2.5, 280, 3.2, 0.35, 1.0, 58,
    dict(arch="rifle", rx=(-0.1, 0.22), rh=0.065, barrel=0.25, hg=(0.18, "wood", "ak"), stock="wood", mag="curved",
         magx=0.08, sight="iron", furn="wood", muzzle="brake", gastube=True))
gun("HK416", "HK416", "Epic", "Rifle", 30, 0.08, True, 30, 90, 2.2, 300, 2.8, 0.2, 0.8, 55,
    dict(arch="rifle", rx=(-0.1, 0.18), rh=0.07, barrel=0.24, hg=(0.28, "poly", "rail"), stock="retract", mag="straight",
         magx=0.07, sight="reddot", furn="poly", muzzle="flash"), "Forsvarets standardgevær i dag.")
gun("Garand", "M1 Garand", "Uncommon", "Rifle", 50, 0.22, False, 8, 32, 2.0, 350, 2.4, 0.2, 1.8, 50,
    dict(arch="rifle", rx=(-0.1, 0.18), rh=0.055, barrel=0.4, hg=(0.32, "wood", "full"), stock="wood", mag="none",
         sight="iron", furn="wood", muzzle="none", wrist=True))
gun("FAL", "FN FAL", "Epic", "Rifle", 42, 0.11, True, 20, 60, 2.5, 320, 3.0, 0.25, 1.3, 52,
    dict(arch="rifle", rx=(-0.12, 0.22), rh=0.07, barrel=0.3, hg=(0.2, "poly", "round"), stock="fixed", mag="straight",
         magx=0.08, sight="carry", furn="poly", muzzle="flash"))
gun("Kar98k", "Mauser Kar98k", "Uncommon", "Rifle", 75, 1.1, False, 5, 25, 3.0, 400, 2.2, 0.15, 3.0, 45,
    dict(arch="bolt", length=1.1, furn="wood", scope=False), headshot=2.5)
gun("Krag", "Krag-Jørgensen", "Rare", "Rifle", 72, 1.1, False, 5, 25, 3.2, 400, 2.2, 0.15, 3.0, 45,
    dict(arch="bolt", length=1.2, furn="wood", scope=False, sidebox=True), "Norskutviklet geværklassiker fra 1890-tallet.", headshot=2.5)
gun("Musket", "Flintlåsmusket", "Legendary", "Rifle", 120, 0.5, False, 1, 8, 6.0, 120, 3.5, 1.0, 5.0, 60,
    dict(arch="musket"), "Fra før 1859. Ett skudd, så en evighet med lading. Men for et skudd.", max_reserve=20, headshot=2.0)
# --- Snipers
gun("SVD", "Dragunov SVD", "Epic", "Sniper", 70, 0.4, False, 10, 30, 2.8, 600, 4.0, 0.1, 2.6, 25,
    dict(arch="rifle", rx=(-0.12, 0.24), rh=0.065, barrel=0.45, hg=(0.22, "wood", "ak"), stock="thumbhole", mag="straight",
         magx=0.1, sight="scope", furn="wood", muzzle="flash"), headshot=2.5)
gun("L96", "L96 AWM", "Legendary", "Sniper", 95, 1.3, False, 5, 20, 3.5, 800, 5.0, 0.05, 4.0, 15,
    dict(arch="bolt", length=1.2, furn="green", scope=True, bipod=True), headshot=2.6)
gun("Barrett", "Barrett M82", "Mythic", "Sniper", 140, 0.6, False, 10, 20, 4.0, 1000, 6.0, 0.08, 6.0, 18,
    dict(arch="rifle", rx=(-0.25, 0.35), rh=0.11, barrel=0.6, hg=(0.0, "steel", "none"), stock="fixed", mag="straight",
         magx=0.12, sight="scope", furn="steel", muzzle="bigbrake", bipod=True, scale=1.2), "Gjennom vegger nesten.", headshot=2.0)
# --- Shotguns
gun("Pump", "Pumpehagle", "Uncommon", "Shotgun", 13, 0.9, False, 6, 24, 3.2, 40, 5.0, 3.5, 3.5, 70,
    dict(arch="shotgun", kind="pump"), mode="pellets", pellets=9)
gun("DoubleBarrel", "Dobbeltløpet hagle", "Common", "Shotgun", 15, 0.25, False, 2, 16, 2.4, 35, 5.5, 4.0, 4.0, 72,
    dict(arch="shotgun", kind="double"), mode="pellets", pellets=9)
gun("Saiga", "Saiga-12", "Epic", "Shotgun", 11, 0.25, False, 8, 24, 2.8, 40, 5.0, 3.5, 2.8, 70,
    dict(arch="rifle", rx=(-0.12, 0.22), rh=0.07, barrel=0.3, hg=(0.18, "poly", "ak"), stock="fixed", mag="box12",
         magx=0.08, sight="iron", furn="poly", muzzle="none", gastube=True, barrel_r=0.012), mode="pellets", pellets=8)
# --- Machine guns
gun("MG3", "MG3", "Legendary", "LMG", 36, 0.05, True, 100, 200, 6.0, 300, 4.5, 1.2, 0.9, 58,
    dict(custom="SM_MG3", muzzle=0.82, sight=0.06), "Hitlers beltesag, fortsatt i Forsvaret. 1200 skudd i minuttet.")
gun("Bren", "Bren", "Epic", "LMG", 40, 0.12, True, 30, 120, 3.0, 300, 4.0, 0.8, 1.0, 58,
    dict(custom="SM_Bren", muzzle=0.73, sight=0.06))
gun("PKM", "PKM", "Legendary", "LMG", 38, 0.085, True, 100, 200, 5.5, 300, 4.2, 1.0, 1.0, 58,
    dict(custom="SM_PKM", muzzle=0.77, sight=0.06))
gun("Minimi", "FN Minimi", "Epic", "LMG", 30, 0.07, True, 100, 200, 5.5, 280, 4.0, 1.0, 0.8, 58,
    dict(arch="rifle", rx=(-0.15, 0.25), rh=0.08, barrel=0.35, hg=(0.15, "poly", "round"), stock="fixed", mag="boxbelt",
         magx=0.06, sight="iron", furn="poly", muzzle="flash", bipod=True, carryhandle=True))
# --- Launchers and bows
gun("CarlGustaf", "Carl Gustaf M4", "Mythic", "Launcher", 150, 1.0, False, 1, 3, 4.5, 400, 3.0, 0.5, 8.0, 50,
    dict(arch="launcher"), "Rekylfri kanon. Blåser opp alt innen 6 meter.", mode="projectile", proj_speed=90,
    proj_gravity=0.1, radius=6.0, max_reserve=6)
gun("M79", "M79 granatkaster", "Legendary", "Launcher", 110, 0.8, False, 1, 8, 2.6, 300, 3.0, 1.0, 5.0, 60,
    dict(arch="m79"), mode="projectile", proj_speed=60, proj_gravity=1.0, radius=5.0, max_reserve=12)
gun("Crossbow", "Armbrøst", "Rare", "Bow", 90, 0.5, False, 1, 15, 2.2, 200, 2.0, 0.2, 1.5, 55,
    dict(arch="crossbow"), "Lydløs. Pilene faller med avstand.", mode="projectile", proj_speed=80, proj_gravity=0.5,
    radius=0.0, max_reserve=30, silent=True, headshot=2.5)
# --- Melee
melee("Knife", "Kampkniv", "Common", 45, 0.45, 1.6, dict(arch="knife", blade=0.17))
melee("Machete", "Machete", "Uncommon", 55, 0.55, 1.8, dict(arch="knife", blade=0.42, wide=True))
melee("Katana", "Katana", "Legendary", 85, 0.6, 2.2, dict(custom="SM_Katana", muzzle=0.8, sight=0.0))
melee("VikingSword", "Vikingsverd", "Epic", 75, 0.65, 2.0, dict(custom="SM_VikingSword", muzzle=0.8, sight=0.0))
melee("VikingAxe", "Vikingsøks", "Epic", 80, 0.75, 1.9, dict(arch="axe", handle=0.75, head="viking"))
melee("BaseballBat", "Baseballkølle", "Common", 40, 0.55, 1.9, dict(arch="bat"))
melee("Crowbar", "Brekkjern", "Common", 42, 0.55, 1.8, dict(arch="crowbar"))
melee("Sledgehammer", "Slegge", "Rare", 95, 1.1, 2.0, dict(arch="axe", handle=0.85, head="hammer"))
melee("FryingPan", "Stekepanne", "Uncommon", 38, 0.5, 1.7, dict(arch="pan"), "Klonk.")

# ------------------------------------------------------------------ armor (body) and helmets
def armor(id, name, rarity, points, absorb, speed, model, desc="", slot="Body"):
    return _add("Armor" if slot == "Body" else "Helmet", id, name, rarity, desc, armor=points, absorb=absorb,
                speed=speed, model=model)


armor("SoftVest", "Myk vest", "Common", 40, 0.35, 1.0, dict(arch="vest", color=(0.08, 0.1, 0.2), plates=0, pouches=1),
      "Kevlar under jakka. Tar av for pistoler.")
armor("TacVest", "Taktisk vest", "Uncommon", 60, 0.45, 0.98, dict(arch="vest", color=(0.04, 0.04, 0.045), plates=1, pouches=3))
armor("HeavyVest", "Tung platevest", "Rare", 85, 0.55, 0.95, dict(arch="vest", color=(0.2, 0.22, 0.12), plates=2, pouches=4))
armor("PlateCarrier", "Plate carrier (keramikk)", "Epic", 110, 0.6, 0.92,
      dict(arch="vest", color=(0.33, 0.28, 0.18), plates=2, pouches=6, collar=True))
armor("Exo", "Eksoskjelett (NTNU-prototype)", "Mythic", 160, 0.7, 1.05, dict(arch="exo"),
      "Bygget av robotikk-gjengen på Elektro. Gjør deg raskere OG tankere.")
armor("BikeHelmet", "Sykkelhjelm", "Common", 15, 0.25, 1.0, dict(arch="helmet", kind="bike"), slot="Head")
armor("CombatHelmet", "Militærhjelm", "Rare", 35, 0.45, 1.0, dict(arch="helmet", kind="combat"), slot="Head")
armor("TacHelmet", "Taktisk hjelm med visir", "Legendary", 60, 0.6, 0.98, dict(arch="helmet", kind="visor"), slot="Head")

# ------------------------------------------------------------------ boosts / drugs (20)
def drug(id, name, rarity, use, effect, model, desc="", heal=0):
    return _add("Drug", id, name, rarity, desc, use=use, effect=effect, heal=heal, model=model)


drug("TurboPill", "Turbo", "Rare", 1.5, "Turbo", dict(arch="pills", color=(0.9, 0.9, 0.95), cap=(0.1, 0.3, 0.9)),
     "Raskere, ingen stamina-bruk, raskere lading. Så kommer krasjet.")
drug("GronnJar", "Grønn", "Uncommon", 3.0, "Gronn", dict(arch="jar", color=(0.2, 0.5, 0.15)),
     "Rolig sikte og litt helse. Treg og sulten.")
drug("Snus", "Snus", "Common", 1.0, "Snus", dict(arch="tin", color=(0.1, 0.25, 0.6)), "Litt raskere omlading.")
drug("Adrenaline", "Adrenalinsprøyte", "Rare", 1.0, "Adrenalin", dict(arch="syringe", color=(0.9, 0.2, 0.1)), heal=35)
drug("Morphine", "Morfin", "Epic", 2.0, "Morfin", dict(arch="syringe", color=(0.8, 0.8, 0.85)), "Tar mindre skade. Alt blir uklart.", heal=20)
drug("Bulk", "Bulk", "Rare", 2.0, "Bulk", dict(arch="pills", color=(0.9, 0.5, 0.1), cap=(0.9, 0.9, 0.9)))
drug("Fokus", "Fokus", "Uncommon", 1.5, "Fokus", dict(arch="blister", color=(0.95, 0.95, 0.3)), "Mindre rekyl og spredning.")
drug("NightEye", "Nattøye", "Rare", 2.0, "Nattoye", dict(arch="dropper", color=(0.2, 0.9, 0.3)), "Se i mørket.")
drug("Lynet", "Lynet", "Uncommon", 1.0, "Lynet", dict(arch="can", color=(0.95, 0.85, 0.1), h=0.12, r=0.028), "Hopp høyere.")
drug("Iskald", "Iskald", "Epic", 1.5, "Iskald", dict(arch="syringe", color=(0.4, 0.8, 1.0)), "Lydløse skritt. Etterpå skjelver du.")
drug("Berserk", "Berserk", "Legendary", 1.0, "Berserk", dict(arch="syringe", color=(0.6, 0.0, 0.0)), "+30% skade, men det spiser helsa di.")
drug("Ghost", "Spøkelse", "Mythic", 2.0, "Spokelse", dict(arch="vial", color=(0.85, 0.9, 1.0)), "Nesten usynlig i 20 sekunder.")
drug("ArmorPill", "Pansertablett", "Rare", 1.5, "Pansertablett", dict(arch="pills", color=(0.4, 0.4, 0.45), cap=(0.1, 0.1, 0.1)))
drug("Regenerator", "Regenerator", "Epic", 2.0, "Regenerator", dict(arch="vial", color=(1.0, 0.3, 0.6)), "3 HP i sekundet.")
drug("Drom", "Drøm", "Uncommon", 1.5, "Drom", dict(arch="blister", color=(0.6, 0.5, 0.9)), "Immun mot blendgranater.")
drug("Oyet", "Øyet (sopp)", "Legendary", 2.5, "Oyet", dict(arch="mushroom"), "Ser alle gjennom vegger i 12 sekunder. Fargene, mann.")
drug("Nicotine", "Nikotinplaster", "Common", 1.0, "Nikotin", dict(arch="patch", color=(0.85, 0.75, 0.6)))
drug("Phoenix", "Føniks", "Mythic", 3.0, "Foniks", dict(arch="vial", color=(1.0, 0.45, 0.0)), "Dør du innen 2 minutter, står du opp igjen.")
drug("Bloodthirst", "Blodtørst", "Legendary", 1.5, "Blodtorst", dict(arch="vial", color=(0.5, 0.0, 0.05)), "25% av skaden du gjør blir helse.")
drug("LaughGas", "Lattergass", "Uncommon", 1.0, "Lattergass", dict(arch="canister", color=(0.7, 0.75, 0.8)))

# ------------------------------------------------------------------ medicine
def med(id, name, rarity, use, heal, effect, model, desc=""):
    return _add("Medicine", id, name, rarity, desc, use=use, heal=heal, effect=effect, model=model)


med("Plaster", "Plaster", "Common", 2.0, 15, "", dict(arch="plaster"))
med("Bandage", "Bandasje", "Uncommon", 3.5, 30, "", dict(arch="bandage"))
med("Painkillers", "Smertestillende", "Common", 1.5, 5, "Smertestillende", dict(arch="pills", color=(1, 1, 1), cap=(0.9, 0.1, 0.1)))
med("Medkit", "Førstehjelpsskrin", "Rare", 6.0, 75, "", dict(arch="medkit"))
med("SurgeryKit", "Feltkirurgsett", "Legendary", 10.0, 100, "Kirurgi", dict(arch="surgery"))

# ------------------------------------------------------------------ food and drink (20+)
def food(id, name, rarity, use, hunger, thirst, heal=0, stamina=0, effect="", model=None, desc="", poison=0.0):
    return _add("Food", id, name, rarity, desc, use=use, hunger=hunger, thirst=thirst, heal=heal, stamina=stamina,
                effect=effect, model=model, poison=poison)


food("Coffee", "Kaffe fra Realfag-kaffebaren", "Common", 2.0, 0, 15, effect="Koffein", model=dict(arch="cup"))
food("EnergyDrink", "Energidrikk", "Uncommon", 2.0, 0, 30, stamina=30, effect="KoffeinPlus",
     model=dict(arch="can", color=(0.1, 0.1, 0.1), band=(0.2, 0.9, 0.2), h=0.16, r=0.03))
food("Water", "Vannflaske", "Common", 2.0, 0, 45, model=dict(arch="bottle", color=(0.7, 0.85, 1.0), cap=(0.1, 0.3, 0.9)))
food("Milk", "Melk (lettmelk)", "Common", 2.0, 10, 30, model=dict(arch="carton", color=(0.95, 0.95, 0.95), band=(0.1, 0.4, 0.9)))
food("OrangeSoda", "Appelsinbrus", "Common", 2.0, 0, 35, heal=5,
     model=dict(arch="bottle", color=(1.0, 0.55, 0.0), cap=(1, 0.8, 0.0)))
food("Baguette", "Baguett", "Common", 3.0, 35, 0, model=dict(arch="baguette"))
food("Lunchbox", "Matpakke med brunost", "Uncommon", 3.0, 40, 0, heal=10, model=dict(arch="lunchbox"))
food("Grandiosa", "Grandiosa", "Rare", 4.0, 70, 0, heal=10, model=dict(arch="pizzabox"), desc="Nasjonalretten.")
food("Kebab", "Kebab i pita", "Uncommon", 3.5, 60, 0, heal=15, model=dict(arch="kebab"))
food("Hotdog", "Pølse i lompe", "Common", 2.5, 30, 0, model=dict(arch="hotdog"))
food("Taco", "Taco (fredag)", "Uncommon", 3.0, 55, 0, heal=5, model=dict(arch="taco"))
food("Skolebolle", "Skolebolle", "Common", 2.0, 20, 0, stamina=30, effect="Sukkerrush", model=dict(arch="bun", color=(0.85, 0.6, 0.3), top=True))
food("Kanelbolle", "Kanelbolle", "Common", 2.0, 20, 0, stamina=25, model=dict(arch="bun", color=(0.65, 0.4, 0.2), swirl=True))
food("Waffle", "Vaffel med syltetøy", "Uncommon", 2.0, 25, 0, stamina=20, model=dict(arch="waffle"))
food("ChocoBar", "Sjokoladekjeks", "Common", 1.5, 15, 0, stamina=40, effect="Sukkerrush", model=dict(arch="bar", color=(0.9, 0.1, 0.1)))
food("Apple", "Eple", "Common", 1.5, 10, 10, heal=5, model=dict(arch="apple"))
food("Banana", "Banan", "Common", 1.5, 15, 0, stamina=20, model=dict(arch="banana"))
food("Noodles", "Instant nudler", "Common", 3.0, 35, -10, model=dict(arch="noodlecup"))
food("FishBalls", "Fiskeboller i boks", "Uncommon", 3.0, 45, 5, model=dict(arch="can", color=(0.9, 0.9, 0.9), band=(0.1, 0.2, 0.6), h=0.09, r=0.045))
food("Salmon", "Røkelaks", "Rare", 3.0, 40, 0, heal=25, model=dict(arch="salmon"), desc="Norsk luksus.")
food("OldPizza", "Kald pizzabit fra Samfundet", "Common", 2.0, 25, 0, effect="Matforgiftning", poison=0.3,
     model=dict(arch="pizzaslice"), desc="30% sjanse for matforgiftning.")
food("Brunost", "Brunost (hel)", "Rare", 3.0, 50, -5, heal=10, stamina=50, model=dict(arch="cheese"))

# ------------------------------------------------------------------ throwables
def throwable(id, name, rarity, kind, dmg, radius, fuse, model, desc=""):
    return _add("Throwable", id, name, rarity, desc, kind=kind, dmg=dmg, radius=radius, fuse=fuse, use=0.3, model=model)


throwable("FragGrenade", "Splintgranat", "Rare", "frag", 120, 6.0, 3.0, dict(arch="grenade", kind="frag"))
throwable("SmokeGrenade", "Røykgranat", "Uncommon", "smoke", 0, 7.0, 1.5, dict(arch="grenade", kind="smoke"))
throwable("Flashbang", "Blendgranat", "Uncommon", "flash", 0, 15.0, 1.5, dict(arch="grenade", kind="flash"))
throwable("Molotov", "Molotovcocktail", "Rare", "fire", 15, 3.5, 0.0, dict(arch="molotov"), "15 skade i sekundet i flammene.")
throwable("C4", "C4", "Legendary", "c4", 200, 7.0, 4.0, dict(arch="c4"), "Fester seg der den lander. Smeller etter 4 sek.")
throwable("TripMine", "Snubletrådmine", "Epic", "mine", 130, 4.0, 0.0, dict(arch="mine"), "Går av når noen andre går forbi.")

# ------------------------------------------------------------------ gadgets and other stuff
def gadget(id, name, rarity, kind, model, desc="", use=1.0, **kw):
    return _add("Gadget", id, name, rarity, desc, kind=kind, use=use, model=model, **kw)


gadget("Binoculars", "Kikkert", "Common", "zoom", dict(arch="binoculars"), "Hold F for å zoome.", use=0.0)
gadget("NVGoggles", "Nattsynsbriller", "Rare", "nightvision", dict(arch="goggles"), "Slå av og på med F.", use=0.0)
gadget("Drone", "Speiderdrone", "Legendary", "drone", dict(arch="drone"), "Viser alle innen 40 m gjennom vegger i 10 sek.", use=1.5)
gadget("Grapple", "Gripekrok", "Epic", "grapple", dict(arch="grapple"), "Skyt deg opp på tak, opptil 35 m.", use=0.0, charges=3)
gadget("Parachute", "Fallskjerm", "Uncommon", "parachute", dict(arch="parachute"), "Hopp fra Sentralbygg 1. Svev ned.", use=0.5)
gadget("Keycard", "NTNU-adgangskort", "Rare", "key", dict(arch="keycard"), "Åpner låste kasser og kodelåser på dører med en gang.", use=0.5)
gadget("Lockpick", "Dirkesett", "Uncommon", "lockpick", dict(arch="lockpick"), "Åpner låste kasser og kodelåser på 6 sekunder.", use=6.0)
gadget("Flashlight", "Lommelykt", "Common", "light", dict(arch="flashlight"), "Slå av og på med F.", use=0.0)
gadget("Radio", "Walkie-talkie", "Uncommon", "radio", dict(arch="radio"), "Viser hvor det skytes i nærheten.", use=0.0)
gadget("AmmoBox", "Ammokasse", "Common", "ammo", dict(arch="ammobox"), "Fyller på ammo til alle våpnene dine.", use=2.0)
gadget("ArmorPlate", "Panserplate", "Uncommon", "plate", dict(arch="plate"), "Reparerer vesten med 40.", use=3.0)
gadget("SmallBackpack", "Liten sekk", "Uncommon", "backpack", dict(arch="backpack", size=0.8), "+4 plasser i sekken.", use=0.0, slots=4)
gadget("BigBackpack", "Stor turbag", "Rare", "backpack", dict(arch="backpack", size=1.15), "+8 plasser i sekken.", use=0.0, slots=8)
gadget("Suppressor", "Lyddemper", "Rare", "att_suppressor", dict(arch="suppressor"), "Setter på våpenet du holder. Ingen sporlys for andre.", use=2.0)
gadget("RedDot", "Rødpunktsikte", "Uncommon", "att_reddot", dict(arch="reddot"), "Mindre spredning når du sikter.", use=2.0)
gadget("Scope", "Kikkertsikte", "Rare", "att_scope", dict(arch="scope"), "Zoom på våpenet du holder.", use=2.0)
gadget("ExtMag", "Utvidet magasin", "Rare", "att_extmag", dict(arch="extmag"), "+50% magasin.", use=2.0)
gadget("Laser", "Laser", "Uncommon", "att_laser", dict(arch="laser"), "Bedre treff fra hofta.", use=2.0)
gadget("Compass", "Kompass", "Common", "compass", dict(arch="compass"), "Viser kompass i HUD-en.", use=0.0)
gadget("StudentCard", "Studentbevis", "Common", "joke", dict(arch="keycard", color=(0.1, 0.4, 0.8)), "Gir 10% rabatt i kantina. Ellers ubrukelig.", use=0.0)
gadget("Defib", "Hjertestarter", "Legendary", "defib", dict(arch="defib"), "Gir deg Føniks-effekten: stå opp igjen hvis du dør.", use=4.0, effect="Foniks")
gadget("Trophy", "NTNU-pokal", "Mythic", "joke", dict(arch="trophy"), "Helt ubrukelig. Men du er den eneste som har den.", use=0.0)


# ================================================================== ROUND 3: more of everything
# --- more guns
gun("OverUnder", "Over/under-hagle (konkurranse)", "Epic", "Shotgun", 16, 0.22, False, 2, 20, 2.2, 45, 4.5, 2.8, 3.8, 70,
    dict(arch="overunder"), "Gravert stålkasse og valnøttskjefte. To skudd, så pent du kan.", mode="pellets", pellets=10)
gun("M72", "M72 LAW", "Epic", "Launcher", 130, 1.0, False, 1, 0, 2.0, 300, 3.0, 0.6, 7.0, 55,
    dict(arch="m72"), "Engangs rakettgevær fra Forsvaret. Ett skudd, så kaster du det.", mode="projectile",
    proj_speed=110, proj_gravity=0.15, radius=5.0, max_reserve=0)
gun("Mosin", "Mosin-Nagant", "Uncommon", "Rifle", 74, 1.15, False, 5, 25, 3.2, 400, 2.2, 0.15, 3.0, 45,
    dict(arch="bolt", length=1.23, furn="wood", scope=False), headshot=2.5)
gun("Rem700", "Jaktrifle med kikkert", "Rare", "Sniper", 82, 1.2, False, 4, 20, 3.0, 600, 4.0, 0.08, 3.2, 22,
    dict(arch="bolt", length=1.08, furn="wood", scope=True), "Elgjakt-favoritten. Treffer det du sikter på.", headshot=2.5)
gun("M14", "M14", "Rare", "Rifle", 44, 0.12, True, 20, 60, 2.6, 350, 3.0, 0.2, 1.4, 50,
    dict(arch="rifle", rx=(-0.1, 0.2), rh=0.06, barrel=0.38, hg=(0.3, "wood", "full"), stock="wood", mag="straight",
         magx=0.08, sight="iron", furn="wood", muzzle="flash", wrist=True))
gun("G36", "G36", "Rare", "Rifle", 30, 0.085, True, 30, 90, 2.3, 300, 3.0, 0.25, 0.85, 56,
    dict(arch="rifle", rx=(-0.1, 0.2), rh=0.075, barrel=0.22, hg=(0.22, "poly", "round"), stock="fixed", mag="straight",
         magx=0.08, sight="carry", furn="poly", muzzle="flash", carryhandle=True))
gun("SCAR", "SCAR-H", "Legendary", "Rifle", 40, 0.095, True, 20, 80, 2.4, 350, 2.8, 0.2, 1.1, 52,
    dict(arch="rifle", rx=(-0.1, 0.2), rh=0.075, barrel=0.26, hg=(0.28, "tan", "rail"), stock="retract", mag="straight",
         magx=0.08, sight="reddot", furn="tan", muzzle="flash"), "Tung 7,62 med rødpunkt fra fabrikken.")
gun("UMP", "UMP45", "Uncommon", "SMG", 24, 0.1, True, 25, 75, 2.2, 60, 2.6, 0.6, 0.8, 70,
    dict(arch="rifle", rx=(-0.08, 0.18), rh=0.07, barrel=0.06, hg=(0.12, "poly", "round"), stock="retract", mag="straight",
         magx=0.08, sight="iron", furn="poly", muzzle="none"))
gun("P90", "P90", "Epic", "SMG", 18, 0.055, True, 50, 100, 2.6, 60, 2.6, 0.6, 0.5, 70,
    dict(arch="p90"), "50 skudd på toppen av våpenet.")
gun("Vector", "Kriss Vector", "Legendary", "SMG", 22, 0.05, True, 25, 75, 2.0, 55, 2.4, 0.5, 0.35, 70,
    dict(arch="rifle", rx=(-0.08, 0.16), rh=0.09, barrel=0.07, hg=(0.08, "poly", "rail"), stock="retract", mag="straight",
         magx=0.03, sight="reddot", furn="poly", muzzle="none"), "Nesten ingen rekyl.")
gun("FlareGun", "Signalpistol", "Uncommon", "Launcher", 25, 1.0, False, 1, 4, 2.0, 120, 2.0, 1.0, 2.0, 70,
    dict(arch="flare"), "Lyser opp himmelen. Og setter fyr på folk.", mode="projectile", proj_speed=45, proj_gravity=0.6,
    radius=2.0, max_reserve=8)
gun("Longbow", "Langbue", "Rare", "Bow", 70, 0.9, False, 1, 20, 1.2, 150, 2.5, 0.4, 1.2, 60,
    dict(arch="bow"), "Lydløs og rask å lade.", mode="projectile", proj_speed=65, proj_gravity=0.7, radius=0.0,
    max_reserve=40, silent=True, headshot=2.5)
gun("PrintPistol", "3D-printet pistol", "Uncommon", "Pistol", 30, 0.5, False, 2, 6, 3.0, 25, 3.5, 1.2, 2.5, 78,
    dict(arch="printed"), "Laget på en 3D-printer på NTNU. Holder kanskje.")
melee("DaneAxe", "Tobladet stridsøks", "Legendary", 110, 0.95, 2.2, dict(arch="daneaxe", handle=0.9),
      "Dobbelt så mange blader. Dobbelt så mye vikinger.")
melee("Spear", "Spyd", "Rare", 70, 0.7, 2.8, dict(arch="spear"), "Lengst rekkevidde av alt.")
melee("PrintKnife", "3D-printet kniv", "Common", 30, 0.45, 1.5, dict(arch="printknife"))

# --- armor
armor("PrintHelmet", "3D-printet hjelm", "Uncommon", 35, 0.3, 1.0, dict(arch="helmet", kind="bike"), slot="Helmet")

# --- ammo per caliber. Picking it up fills every gun you carry that uses it; the rest waits in the sack for later.
CALIBERS = {
    # id: (name, rounds per box, colour, model params)
    "9mm":   ("9 mm", 30, (0.2, 0.5, 0.95), dict(len=0.02, r=0.0045)),
    "45acp": (".45 ACP", 21, (0.95, 0.75, 0.1), dict(len=0.022, r=0.0057)),
    "357":   (".357 / .50 AE magnum", 12, (0.75, 0.1, 0.1), dict(len=0.03, r=0.0058)),
    "556":   ("5,56 mm", 30, (0.2, 0.75, 0.25), dict(len=0.045, r=0.0045, tip=(0.15, 0.5, 0.2))),
    "762x39": ("7,62 x 39 mm", 30, (0.85, 0.45, 0.1), dict(len=0.04, r=0.0055)),
    "762nato": ("7,62 x 51 mm NATO", 20, (0.55, 0.55, 0.15), dict(len=0.05, r=0.006)),
    "762x54": ("7,62 x 54R", 20, (0.6, 0.3, 0.6), dict(len=0.053, r=0.006)),
    "57":    ("5,7 mm", 50, (0.1, 0.8, 0.8), dict(len=0.04, r=0.0038)),
    "12g":   ("12-gauge hagl", 10, (0.85, 0.1, 0.08), dict(len=0.06, r=0.0095, shell=True, n=4)),
    "50bmg": (".50 BMG", 10, (0.1, 0.1, 0.1), dict(len=0.1, r=0.01, n=3)),
    "40mm":  ("40 mm granater", 3, (0.5, 0.55, 0.3), dict(len=0.08, r=0.02, n=2)),
    "rocket": ("84 mm raketter", 1, (0.3, 0.35, 0.2), dict(len=0.2, r=0.04, n=1)),
    "arrow": ("Piler og bolter", 10, (0.5, 0.35, 0.2), dict(len=0.3, r=0.004, n=6)),
    "flare": ("Signalpatroner", 4, (0.95, 0.4, 0.05), dict(len=0.04, r=0.012, shell=True, n=3)),
    "ball":  ("Kruttkuler", 6, (0.3, 0.3, 0.3), dict(len=0.012, r=0.008, n=6)),
}
# which caliber each gun uses
GUN_CALIBER = {
    "M1911": "45acp", "Glock17": "9mm", "Makarov": "9mm", "Luger": "9mm", "Revolver": "357", "DesertEagle": "357",
    "PistolKnife": "9mm", "MP40": "9mm", "Uzi": "9mm", "Suomi": "9mm", "MP5": "9mm", "Thompson": "45acp",
    "PPSh": "9mm", "AG3": "762nato", "AK47": "762x39", "HK416": "556", "Garand": "762nato", "FAL": "762nato",
    "Kar98k": "762x54", "Krag": "762x54", "Musket": "ball", "SVD": "762x54", "L96": "762nato", "Barrett": "50bmg",
    "Pump": "12g", "DoubleBarrel": "12g", "Saiga": "12g", "MG3": "762nato", "Bren": "762nato", "PKM": "762x54",
    "Minimi": "556", "CarlGustaf": "rocket", "M79": "40mm", "Crossbow": "arrow", "OverUnder": "12g", "M72": "",
    "Mosin": "762x54", "Rem700": "762nato", "M14": "762nato", "G36": "556", "SCAR": "762nato", "UMP": "45acp",
    "P90": "57", "Vector": "45acp", "FlareGun": "flare", "Longbow": "arrow", "PrintPistol": "9mm",
}
for _it in ITEMS:
    if _it["cat"] == "Weapon" and _it["mode"] != "melee":
        _it["caliber"] = GUN_CALIBER.get(_it["id"], "")

for _cal, (_name, _rounds, _color, _mp) in CALIBERS.items():
    _rar = "Uncommon" if _cal in ("50bmg", "40mm", "rocket") else "Common"
    _id = "Ammo_" + _cal
    _users = ", ".join(i["name"].split(" (")[0] for i in ITEMS if i.get("caliber") == _cal)
    gadget(_id, f"Ammo: {_name}", _rar, "caliber", dict(arch="cal", color=_color, **_mp),
           f"{_rounds} skudd. Passer i: {_users}.", use=0.0, caliber=_cal, rounds=_rounds)

# --- special ammo: loads the gun you hold with a special kind of round
gadget("AmmoAP", "Panserbrytende ammo", "Rare", "ammotype_ap", dict(arch="specialammo", color=(0.2, 0.3, 0.9)),
       "Går gjennom vester og hjelmer. Bruk med F på våpenet du holder.", use=2.0)
gadget("AmmoHP", "Hulspissammo", "Uncommon", "ammotype_hp", dict(arch="specialammo", color=(0.9, 0.2, 0.2)),
       "+30% skade mot folk uten vest, mindre mot vest.", use=2.0)
gadget("AmmoFire", "Brannammo", "Epic", "ammotype_fire", dict(arch="specialammo", color=(1.0, 0.5, 0.05)),
       "Treff setter fyr på folk i 3 sekunder.", use=2.0)
gadget("AmmoTracer", "Lysspor", "Common", "ammotype_tracer", dict(arch="specialammo", color=(0.95, 0.95, 0.2)),
       "Kulene lyser, så du ser hvor de går. Fienden ser også hvor du skyter fra.", use=2.0)
gadget("AmmoShock", "Elektrosjokk-ammo", "Legendary", "ammotype_shock", dict(arch="specialammo", color=(0.4, 0.9, 1.0)),
       "Treff lammer i 3 sekunder.", use=2.0)

# --- more attachments
gadget("Holo", "Holografisk sikte", "Rare", "att_holo", dict(arch="holo"), "Raskt sikte med litt zoom.", use=2.0)
gadget("Scope8x", "8x kikkertsikte", "Epic", "att_scope8x", dict(arch="scope8"), "For snikskyttere. Kraftig zoom.", use=2.0)
gadget("Drum", "Trommelmagasin", "Epic", "att_drum", dict(arch="drum"), "Dobbelt så stort magasin, litt tregere lading.", use=2.0)
gadget("Compensator", "Kompensator", "Uncommon", "att_comp", dict(arch="comp"), "-30% rekyl. Bråker mer.", use=2.0)
gadget("Grip", "Fremre håndtak", "Uncommon", "att_grip", dict(arch="vgrip"), "-15% rekyl og bedre treff fra hofta.", use=2.0)

# --- 3D printing
gadget("Filament", "Filament-spole", "Common", "filament", dict(arch="spool"),
       "Bruk den i en 3D-printer for å lage ting.", use=0.0)
gadget("PrintDrone", "3D-printet drone", "Epic", "drone", dict(arch="drone"), "Hjemmelaget speiderdrone. Ser 40 m i 10 sek.", use=1.5)

# --- more food
food("Lefse", "Lefse med smør og sukker", "Common", 2.0, 25, 0, stamina=20, model=dict(arch="lefse"))
food("Farikal", "Fårikål på boks", "Uncommon", 4.0, 65, 5, heal=10, effect="Mett",
     model=dict(arch="can", color=(0.85, 0.85, 0.8), band=(0.2, 0.45, 0.15), h=0.11, r=0.05), desc="Nasjonalretten (den andre).")
food("Pinnekjott", "Pinnekjøtt", "Rare", 4.0, 70, -10, heal=20, effect="Mett", model=dict(arch="lunchbox"))
food("Julebrus", "Julebrus", "Uncommon", 1.5, 0, 35, stamina=25, effect="Julestemning",
     model=dict(arch="bottle", color=(0.8, 0.1, 0.1), cap=(0.9, 0.8, 0.2)))
food("Risgrot", "Risengrynsgrøt", "Common", 3.0, 45, 0, heal=5, model=dict(arch="noodlecup"))
food("Tran", "Tran", "Uncommon", 1.0, 0, 0, heal=10, effect="Tran", model=dict(arch="bottle", color=(0.9, 0.75, 0.2), cap=(0.1, 0.3, 0.7)),
     desc="Vond, men sunn.")
food("Kvikklunsj", "Turkjeks", "Common", 1.5, 15, 0, stamina=45, effect="Sukkerrush", model=dict(arch="bar", color=(0.95, 0.85, 0.1)))
food("Solbaer", "Solbærtoddy", "Common", 2.0, 0, 25, heal=5, model=dict(arch="cup"))
food("Rakfisk", "Rakfisk", "Rare", 3.0, 50, 0, heal=15, effect="Matforgiftning", poison=0.5, model=dict(arch="salmon"),
     desc="Halvparten blir syke. Den andre halvparten blir sterke.")
food("Sodd", "Trøndersodd", "Rare", 4.0, 60, 20, heal=20, effect="Mett", model=dict(arch="noodlecup"),
     desc="Trondheims stolthet.")


def items(cat=None):
    return [i for i in ITEMS if cat is None or i["cat"] == cat]


if __name__ == "__main__":
    from collections import Counter
    print(Counter(i["cat"] for i in ITEMS))
    print(len(ITEMS), "items,", len(EFFECTS), "effects")
    for i in ITEMS:
        e = i.get("effect")
        assert not e or e in EFFECTS, (i["id"], e)
    for k, e in EFFECTS.items():
        assert not e.get("after") or e["after"] in EFFECTS, k
