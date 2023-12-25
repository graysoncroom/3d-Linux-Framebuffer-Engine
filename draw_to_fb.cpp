#include <string>
#include <iostream>
#include <cmath> // std::sqrt

#include <linux/fb.h> // ioctl constants and structures we use come from here
#include <unistd.h> // close
#include <sys/ioctl.h> // ioctl
#include <fcntl.h> // open
#include <sys/mman.h> // mmap

#include <vector>

// ========== Constants ==========

// ========== Documentation ==========
// {{{
/* Put a `GeometricGameObject` into the global `game_objects`, which is of type `std::vector`,
 * to have it rendered to the framebuffer.
 *
 * A `GeoemtricGameObject` is an object living in the 3d world you are exploring.
 * It is made up of:
 *   (1) A function f : R^3 -> R, which is defined as (x,y,z) |-> a, i.e. a = f(x,y,z)
 *   (2) A color
 *
 *   Remark: f is a function pointer taking 3 ints (x,y,z) and returning an int (a)
 *   Remark: (2) is of type Color::Color as defined above
 *
 * Definition: The _Frame_Buffer_ is the viewport in a Linux TTY session. It is the two
 *  dimensional screen that we can draw on.
 *
 * Remark: One can think about the Frame Buffer as a section of an R^2 space that all
 *  `GeometricGameObject`s must eventually mapped to if they are to be seen by the player.
 *
 * Definition: The _Camera_ is the plane, whose type is a `GeometricGameObject`, 
 *  representing our Frame Buffer in the Game World.
 *
 * Remark: In order to view a `GeometricGameObject`, we must project the object onto the 
 *  Camera.
 *
 */
// }}}


namespace Color {
  struct Color {
    int red;
    int green;
    int blue;
  
  };
  
  static constexpr Color RED{255, 0, 0};
  static constexpr Color GREEN{0, 255, 0};
  static constexpr Color BLUE{0, 0, 255};
};


struct GeometricGameObject {
  int(*fn)(int, int, int);
  Color::Color color;
};

class Framebuffer { 
// {{{
 private:
  int bpp;
  int bytes;
  int fb_memory_size;
  char *fb_memory;

 public:
  int width;
  int height;

  Framebuffer() {
    // fbfd = frame buffer file descriptor
    int fbfd = open("/dev/fb0", O_RDWR);
    if (fbfd == -1) {
      std::cerr << "Error opening framebuffer 0." << std::endl;
    }

    // Ask the linux kernel for information about our framebuffer (e.g. width, height, etc...)
    // The information gets stored in our local fb_var_screeninfo struct
    struct fb_var_screeninfo vinfo;
    ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo);

    // Extract info we want from struct into local variables
    width = vinfo.xres;
    height = vinfo.yres;
    bpp = vinfo.bits_per_pixel; // bpp = bits per pixel
    bytes = bpp / 8;

    //std::cout << "fb_width = " << fb_width << "\n";
    //std::cout << "fb_height = " << fb_height << "\n";
    //std::cout << "fb_bpp = " << fb_bpp << "\n";
    //std::cout << "fb_bytes = " << fb_bytes << "\n";

    fb_memory_size = width * height * bytes;

    // map framebuffer device into memory
    fb_memory = (char*)mmap(0, fb_memory_size, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, (off_t)0);
    if (fb_memory == MAP_FAILED) {
      std::cerr << "mmap failed\n";
      exit(0);
      //std::cerr << "mmap return value = " << fb_memory << std::endl;
    }
    
    // file descriptor can be closed after the mmap syscall without affecting the mapping
    // for more information, see the `mmap` manpage
    //close(fbfd);
  }

  void write(int x, int y, Color::Color c) {
    int offset = (y * width + x) * 4;
    fb_memory[offset + 0] = c.blue;
    fb_memory[offset + 1] = c.green;
    fb_memory[offset + 2] = c.red;
    fb_memory[offset + 3] = 0; // Remark: This may not be needed.
  }

  ~Framebuffer() {
    munmap(fb_memory, fb_memory_size);
  }
// }}}
}; 

class Camera : public FrameBuffer, public GeoemtricGameObject {

}


// ========== Global Objects ==========
std::vector<GeometricGameObject> game_objects;
Framebuffer fb;


// ========== Global Functions ==========
int square_fn(int x, int y, int z) {
  return (x >= 0 && x < 500) &&
         (y >=0 && y < 500) ? 
         0 : 1;
}

int circle_fn(int x, int y, int z) {
  static int radius = 200;
  int translated_x = x - 1500;
  int translated_y = y - 400;
  int translated_x_squared = translated_x*translated_x;
  int translated_y_squared = translated_y*translated_y;
  return std::sqrt(translated_x_squared + translated_y_squared) - radius;
}

void draw_screen(int x, int y) {
  for (GeometricGameObject obj : game_objects) {
    // res = f(x, y, z)
    int res = obj.fn(x, y, 0);
    //std::cout << "obj.fn(x, y, 0) = " << res << std::endl;
    if (res == 0) {

      fb.write(x, y, obj.color);
    }
  }
}


// ========== Main Loop ==========
int main() {
  // Create game objects
  GeometricGameObject square;
  square.fn = &square_fn;
  square.color = Color::RED;
  game_objects.push_back(square);

  GeometricGameObject circle;
  circle.fn = &circle_fn;
  circle.color = Color::GREEN;
  game_objects.push_back(circle);

  // Start Game Loop
  while (true) {
    for (int x = 0; x < fb.width; ++x) {
      for (int y = 0; y < fb.height; ++y) {
        draw_screen(x, y);
      }
    }
  }

  return 0;
}
