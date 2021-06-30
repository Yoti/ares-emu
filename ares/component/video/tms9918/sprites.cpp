auto TMS9918::sprite(n8 voffset) -> void {
  io.spriteCollision = false;
  io.spriteOverflow = false;
  io.spriteOverflowIndex = 0;

  n8 valid = 0;
  n5 limit = (8 << io.spriteSize << io.spriteZoom) - 1;
  for(u32 index : range(4)) sprites[index].y = 0xd0;

  n14 attributeAddress;
  attributeAddress.bit(7,13) = io.spriteAttributeTableAddress;
  for(u32 index : range(32)) {
    n8 y = vram.read(attributeAddress++);
    if(y == 0xd0) break;

    n8 x = vram.read(attributeAddress++);
    n8 pattern = vram.read(attributeAddress++);
    n8 extra = vram.read(attributeAddress++);

    if(extra.bit(7)) x -= 32;
    y += 1;
    if(voffset < y) continue;
    if(voffset > y + limit) continue;

    if(limit == 15) pattern.bit(0,1) = 0;

    if(valid == 4) {
      io.spriteOverflow = true;
      io.spriteOverflowIndex = index;
      break;
    }

    sprites[valid++] = {x, y, pattern, extra.bit(0,3)};
  }
}

auto TMS9918::sprite(n8 hoffset, n8 voffset) -> void {
  n4 color;
  n5 limit = (8 << io.spriteSize << io.spriteZoom) - 1;

  for(u32 n : range(4)) {
    auto& o = sprites[n];
    if(o.y == 0xd0) continue;
    if(hoffset < o.x) continue;
    if(hoffset > o.x + limit) continue;

    u32 x = hoffset - o.x >> io.spriteZoom;
    u32 y = voffset - o.y >> io.spriteZoom;

    n14 address;
    address.bit( 0,10) = (o.pattern << 3) + (x >> 3 << 4) + y;
    address.bit(11,13) = io.spritePatternTableAddress;

    n3 index = x ^ 7;
    if(vram.read(address).bit(index)) {
      if(color) { io.spriteCollision = true; break; }
      color = o.color;
    }
  }

  if(color) output.color = color;
}
