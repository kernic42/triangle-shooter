from PIL import Image

fire = Image.open("fire.png")
ice = Image.open("ice.png")
radioactive = Image.open("radioactive.png")

atlas = Image.new("RGBA", (1024 * 3, 1536))
atlas.paste(fire, (0, 0))
atlas.paste(ice, (1024, 0))
atlas.paste(radioactive, (2048, 0))

atlas.save("atlas.png")
print("Created atlas.png")