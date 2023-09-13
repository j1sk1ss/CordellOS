#include <core/devices/BlockDevice.hpp>

namespace arch {
    namespace i686 {

        class VGATextDevice : public CharacterDevice {
            public:
                VGATextDevice();
                virtual size_t Read(uint8_t* data, size_t size);
                virtual size_t Print(const uint8_t* data, size_t size);

                void Clear();

            private:
                void PutChar(int x, int y, char c);
                void PutColor(int x, int y, uint8_t color);
                char GetChar(int x, int y);
                uint8_t GetColor(int x, int y);

                void SetCursor(int x, int y);
                void Scrollback(int lines);

                void PutChar(char c);

                int _screenX, _screenY;
                static uint8_t* _screenBuffer;
            };
        
    }
}