 ## TinyColor - ATTiny13 RGB LED control using Binary Code Modulation

### Binary Code Modulation (BCM)
BCM is a less known technique for modulating the output to control a frequency insensitive device such as LED. It can be used as a replacement of PWM. 
ATtiny13 MCU has only 2 pwm channels but an RGB LED has 3 pins. So, to chnage color, PWM is not an option here. Here BCM comes to my rescue. This is how BCM works:

Say, I want to pulse out the value 128. So, we'll set the GPIO based on the value of each bit of 128, starting from MSB. After setting the value of each bit (1 or 0), we'll wait N ticks where N is the multiple of the weighted value of that bit's position.

128 -> 1 0 0 0 0 0 0. We'll first set gpio to high as the MSB is 1 and then we'll wait 128 * n (n = 1, 2, 3...) ticks because the weighted value of MSB is 128. Then we'll output the next bit which is 0, so gpio = low. This time, gpio will remain low for 64 * n ticks because weighted value of the 7th bit is 64. This will continue in this way until we hit the LSB and then we'll wait 1 * n tick. Then everything will start from the MSB again.

Let's see the graph for a 4-bit number, say 5.

5  ->  0 1 0 1  ->  \\ . . . . . . . ./ ' ' ' '\\ . ./ '\

To represent 5, the duty cycle should be = 5/(2^4 - 1) * 100% = 33.33%.
```

level     0      1    0 1
          |      |    | |
          v      v    v v
      +        +----+  +-+
      |        |    |  | |
      |        |    |  | |
      +--------+    +--+ +
          ^      ^  ^   ^
          |      |  |   |
ticks     8      4  2   1

So, here duty cycle = (on time / total time) * 100 %
                    = {(4 + 1) ticks / (8 + 4 + 2 + 1) ticks} * 100 %
                    = 33.33 %
```
So, the duty cycle criteria is properly met and the led's brightness will represent value 5.

If PWM was used instead, it would look like this:

```
level   1          0
        |          |
        v          v
      +-----+          +
      |     |          |
      |     |          |
      +     +----------+
         ^       ^
         |       |
ticks    5       10
```

### Learning resources
BCM itself is not that compicated to understand. To learn more on this, read this article: http://www.batsocks.co.uk/readme/art_bcm_1.htm
