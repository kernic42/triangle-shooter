from PIL import Image

textures = ["cannon_orange.png", "cannon_blue.png", "cannon_green.png", ""]
grid_x = 2
grid_y = 2

cell_w, cell_h = None, None
images = []

for t in textures:
    if t:
        img = Image.open(t)
        if cell_w is None:
            cell_w, cell_h = img.size
        images.append((t, img))
    else:
        images.append((None, None))

atlas = Image.new("RGBA", (cell_w * grid_x, cell_h * grid_y))

for i, (name, img) in enumerate(images):
    if img:
        x = (i % grid_x) * cell_w
        y = (i // grid_x) * cell_h
        atlas.paste(img, (x, y))

atlas.save("cannon_atlas.png")
print(f"Created cannon_atlas.png ({cell_w * grid_x}x{cell_h * grid_y})")
input("Press Enter to close")