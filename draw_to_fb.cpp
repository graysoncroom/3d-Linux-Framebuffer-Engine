#include <string>
#include <iostream>
#include <cmath>

#include <linux/fb.h> // ioctl constants and structures we use come from here
#include <unistd.h> // close
#include <sys/ioctl.h> // ioctl
#include <fcntl.h> // open
#include <sys/mman.h> // mmap

#include <vector>

struct GeometricGameObject {
  int(*fn)(int, int, int);
};

class Framebuffer {
//private:
public:
  int width;
  int height;
  int bpp;
  int bytes;

  int fb_memory_size;

  char *fb_memory;

public:
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

  struct Color {
    int red;
    int green;
    int blue;
  };

  static constexpr Color RED{255, 0, 0};
  static constexpr Color GREEN{0, 255, 0};
  static constexpr Color BLUE{0, 0, 255};

  struct Pixel {
    int x;
    int y;
    Color color;
  };

  void write(Pixel& p) {
    int offset = (p.y * width + p.x) * 4;
    fb_memory[offset + 0] = p.color.blue;
    fb_memory[offset + 1] = p.color.green;
    fb_memory[offset + 2] = p.color.red;
    fb_memory[offset + 3] = 0; // Remark: This may not be needed.
  }

  ~Framebuffer() {
    munmap(fb_memory, fb_memory_size);
  }

};

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


std::vector<GeometricGameObject> game_objects;

int main() {
  // Create game objects
  GeometricGameObject x;
  x.fn = &square_fn;
  game_objects.push_back(x);

  GeometricGameObject y;
  y.fn = &circle_fn;
  game_objects.push_back(y);

  // Set up Framebuffer
  Framebuffer fb;
  //int x, y;
  Framebuffer::Pixel p;
  p.color = Framebuffer::RED;
  unsigned long long t = 0;

  // Start Game Loop
  while (true) {
    for (int x = 0; x < fb.width; ++x) {
      for (int y = 0; y < fb.height; ++y) {
        for (GeometricGameObject obj : game_objects) {
          int res = obj.fn(x, y, 0);
          //std::cout << "obj.fn(x, y, 0) = " << res << std::endl;
          if (res == 0) {
            p.x = x;
            p.y = y;
            fb.write(p);
          }
        }
      }
    }
    //p.color = Framebuffer::RED;
    //for (int i = 0; i < 500; ++i) {
    //  for (int j = 0; j < 500; ++j) {
    //    p.x = i;
    //    p.y = j;
    //    fb.write(p);
    //  }
    //}

    //p.color = Framebuffer::GREEN;
    //for (int i = 600; i < 1000; ++i) {
    //  for (int j = 0; j < i - 600; ++j) {
    //    p.x = i;
    //    p.y = j;
    //    fb.write(p);

    //  }
    //}

    // Circle
    // center (1200, 200)
    // (x, y) = (r*cos(theta)+1200, r*sin(theta)+200)
    ////int r = 200;
    //for (int r = 200; r > 150; --r) {
    //  for (int theta = 0; theta < 5000; ++theta) {
    //    p.x = 1300 + r*std::cos(theta);
    //    p.y = 200 + r*std::sin(theta);
    //    fb.write(p);
    //  }
    //}


    // another circle
    // center (1500, 200)
    //for (int i = 1500; i < 1900; ++i) {
    //  for (int j = 0; j < 400; ++j) {
    //    int adj_i = i - 1700;
    //    int adj_j = j - 200;
    //    int ii = adj_i*adj_i;
    //    int jj = adj_j*adj_j;

    //    bool is_within_circle = std::sqrt(ii + jj) < 200;
    //    if (is_within_circle) {
    //      p.x = i;
    //      p.y = j;
    //      fb.write(p);
    //    }
    //  }
    //}

    t += 1;
  }

  return 0;
}
