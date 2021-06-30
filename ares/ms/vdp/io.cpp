auto VDP::vcounter() -> n8 {
  if(Region::NTSCJ() || Region::NTSCU()) {
    switch(io.mode) {
    default:     return io.vcounter <= 218 ? io.vcounter : io.vcounter - 6;  //256x192
    case 0b1011: return io.vcounter <= 234 ? io.vcounter : io.vcounter - 6;  //256x224
    case 0b1110: return io.vcounter;  //256x240
    }
  }

  if(Region::PAL()) {
    switch(io.mode) {
    default:     return io.vcounter <= 242 ? io.vcounter : io.vcounter - 57;  //256x192
    case 0b1011: return io.vcounter <= 258 ? io.vcounter : io.vcounter - 57;  //256x224
    case 0b1110: return io.vcounter <= 266 ? io.vcounter : io.vcounter - 56;  //256x240
    }
  }

  unreachable;
}

auto VDP::hcounter() -> n8 {
  return (io.pcounter - 94) >> 2;
}

auto VDP::hcounterLatch() -> void {
  io.pcounter = io.hcounter;
}

auto VDP::ccounter() -> n12 {
  return io.ccounter;
}

auto VDP::data() -> n8 {
  io.controlLatch = 0;

  auto data = io.vramLatch;
  io.vramLatch = vram[io.address++];
  return data;
}

auto VDP::status() -> n8 {
  io.controlLatch = 0;

  n8 result;
  result.bit(0,4) = io.fifthSprite;
  result.bit(5)   = io.spriteCollision;
  result.bit(6)   = io.spriteOverflow;
  result.bit(7)   = io.intFrame;

  io.intLine = 0;
  io.intFrame = 0;
  io.spriteOverflow = 0;
  io.spriteCollision = 0;
  io.fifthSprite = 0;

  return result;
}

auto VDP::data(n8 data) -> void {
  io.controlLatch = 0;

  if(io.code <= 2) {
    vram[io.address++] = data;
  } else {
    if(Model::MasterSystem()) cram[io.address++ & 0x1f] = data;
    if(Model::GameGear())     cram[io.address++ & 0x3f] = data;
  }
}

auto VDP::control(n8 data) -> void {
  if(io.controlLatch == 0) {
    io.controlLatch = 1;
    io.address.bit(0,7) = data.bit(0,7);
    return;
  } else {
    io.controlLatch = 0;
    io.address.bit(8,13) = data.bit(0,5);
    io.code.bit(0,1) = data.bit(6,7);
  }

  if(io.code == 0) {
    io.vramLatch = vram[io.address++];
  }

  if(io.code == 2) {
    registerWrite(io.address.bit(11,8), io.address.bit(7,0));
  }
}

auto VDP::registerWrite(n4 addr, n8 data) -> void {
  switch(addr) {

  //mode control 1
  case 0x0: {
    io.externalSync = data.bit(0);
    io.mode.bit(1) = data.bit(1);
    io.mode.bit(3) = data.bit(2);
    io.spriteShift = data.bit(3);
    io.lineInterrupts = data.bit(4);
    io.leftClip = data.bit(5);
    io.horizontalScrollLock = data.bit(6);
    io.verticalScrollLock = data.bit(7);
    return;
  }

  //mode control 2
  case 0x1: {
    io.spriteDouble = data.bit(0);
    io.spriteTile = data.bit(1);
    io.mode.bit(2) = data.bit(3);
    io.mode.bit(0) = data.bit(4);
    io.frameInterrupts = data.bit(5);
    io.displayEnable = data.bit(6);
    return;
  }

  //name table base address
  case 0x2: {
    io.nameTableAddress = data.bit(0,3);
    return;
  }

  //color table base address
  case 0x3: {
    io.colorTableAddress = data.bit(0,7);
    return;
  }

  //pattern table base address
  case 0x4: {
    io.patternTableAddress = data.bit(0,2);
    return;
  }

  //sprite attribute table base address
  case 0x5: {
    io.spriteAttributeTableAddress = data.bit(0,6);
    return;
  }

  //sprite pattern table base address
  case 0x6: {
    io.spritePatternTableAddress = data.bit(0,2);
    return;
  }

  //backdrop color
  case 0x7: {
    io.backdropColor = data.bit(0,3);
    return;
  }

  //horizontal scroll offset
  case 0x8: {
    io.hscroll = data.bit(0,7);
    return;
  }

  //vertical scroll offset
  case 0x9: {
    io.vscroll = data.bit(0,7);
    return;
  }

  //line counter
  case 0xa: {
    io.lineCounter = data.bit(0,7);
    return;
  }

  //0xb - 0xf unmapped

  }
}
