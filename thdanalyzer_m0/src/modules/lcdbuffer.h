#ifndef LCDBUFFER_H_
#define LCDBUFFER_H_

class LcdBuffer
{
public:
	char row1[17];
	char row2[17];

	int Width() { return 16; }
};

#endif /* LCDBUFFER_H_ */
