#pragma once

struct BaseStatus {
    
};

// dummy structure for testing
struct CreatureStatus : public BaseStatus {
    CreatureStatus() : x(0), y(0), z(0), heading(0), // dummy
        anim(0), animframe(0), faction(0) { }

    float x;
    float y;
    float z;
    double heading;
    uint8_t anim;
    uint8_t animframe;
    uint8_t faction;		// hostile reticle
    uint16_t equipment1 = 0;	// set equipment model
    uint16_t equipment2 = 0;
    uint16_t equipment3 = 0;
    uint16_t equipment4 = 0;
    uint16_t equipment5 = 0;
    uint16_t equipment6 = 0;
};



