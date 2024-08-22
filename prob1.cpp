//Станбеков Султан Эрнисович
//telegram: @stanbekoov
//почта: stanbekovsulta@gmail.com

#include <iostream>
#include <vector>
#include <cstdint>
// #include <algorithm>

struct image {
    int width, height;
    int format; // 0=GRAY   1=RGB   2=BGR
    uint8_t *data;
};

struct box {
    int x1, y1; 
    int x2, y2;
    int type;   // 0=FACE   1=GUN   2=MASK

    bool merge(box other, float threshold);
    //конструкторы для удобства тестирования
    box() : x1(0), y1(0), x2(0), y2(0), type(-1) {};
    box(int x1, int y1, int x2, int y2, int type) : x1(x1), y1(y1), x2(x2), y2(y2), type(type) {
        if(!(type >= 0 && type <= 2)) {
            throw "invaild type";
        }
        if(x2 < x1 || y2 > y1) {
            throw "invaild coordinates";
        }
    };
};

struct frame {
    image img;
    std::vector<box> boxes;
};

//Данный метод объединяет две рамки, в случае если их площадь пересечения больше threshold и
//имееют один и тот же тип
//возвращает true, если рамки были объединены, иначе false
//результат объединения сохраняется в объект который вызвал метод
bool box::merge(box other, float threshold) {
    if(this->type != other.type) {
        return false;
    }

    box buf;
    
    buf.x1 = std::max(this->x1, other.x1);
    buf.y1 = std::min(this->y1, other.y1);
    buf.x2 = std::min(this->x2, other.x2);
    buf.y2 = std::max(this->y2, other.y2);

    int width = x2 - x1, height = y1 - y2;
    int area = width * height;
    
    if(float(area) < threshold || width < 0 || height < 0) return false;

    (*this) = buf;
    this->type = other.type;
    return true;
}

//функция для объединения всех коробок в массиве
void mergeOverlappingBoxes(std::vector<box>& boxes, float threshold) {
    bool merged = true;

    //цикл идет до первой итерации без объежинения коробок
    while(merged) {
        //данная переменная показывает было ли произведено объежинение в данной итерации
        merged = false;
        std::vector<box> new_boxes;

        //выбираем коробку с которой будет сверять все другие
        for(size_t i = 0; i < boxes.size(); i++) {
            bool currBoxMerged = false;

            //и коробку с которой будем сверять первую
            for(size_t j = i + 1; j < boxes.size(); j++) {
                //если коробки были объединены, удаляем вторую
                if(boxes[i].merge(boxes[j], threshold)) {
                    boxes.erase(boxes.begin() + j);
                    j--;
                    currBoxMerged = true;
                    merged = true;
                }
            }
            new_boxes.push_back(boxes[i]);
            
            //если первая коробка была объединена с какой-то другой, удаляем её
            if(currBoxMerged) {
                boxes.erase(boxes.begin() + i);
                i--;
            }
        }
        boxes = new_boxes;
    }
    boxes.shrink_to_fit();
}

//сначала проверям, соответствует ли изображение rgb-формату
//если нет, возвращаем false, иначе меняем местами значения blue, red
bool rgb2bgr(image &img) { 
    if(img.format != 1) return false;
    int total = img.height * img.width;

    //если количество элементов не делится на 3 возращаем исключение 
    if(total % 3 != 0) {
        throw "Invalid rgb data";
    }
    int r, b;

    for(int i = 0; i < total; i += 3) {
        r = i;
        b = i + 2;
        std::swap(img.data[r], img.data[b]);
    }

    return true;
}

void frame_clean(frame& f, float threshold) {
    //распределяем рамки по типу 
    std::vector<box> newBoxes[3];
     
    for(size_t i = 0; i < f.boxes.size(); i++) {
        newBoxes[f.boxes[i].type].push_back(f.boxes[i]);
    }

    //вызываме mergeOverlappingBoxes для каждого типа и считаем размер
    size_t total = 0, ptr = 0;
    for(size_t i = 0; i < 3; i++) {
        mergeOverlappingBoxes(newBoxes[i], threshold);
        total += newBoxes[i].size();
    }

    //сохраняем результат в изначальный объект
    f.boxes.resize(total);
    for(size_t i = 0; i < 3; i++) {
        for(size_t j = 0; j < newBoxes[i].size(); j++) {
            f.boxes[ptr] = newBoxes[i][j];
            ptr++;
        }
    }
}

frame union_frames(frame& f1, frame& f2, float threshold) { 
    //создаем копии изначальный объектов, что бы не изменить их
    frame f1_copy = f1, f2_copy = f2;

    //"чистим" обе рамки по отдельности
    frame_clean(f1_copy, threshold);
    frame_clean(f2_copy, threshold);

    //добавляем рамки из второй копии в первую
    f1_copy.boxes.resize(f1_copy.boxes.size() + f2_copy.boxes.size());

    for(size_t i = 0; i < f2_copy.boxes.size(); i++) {
        f1_copy.boxes[f1.boxes.size() + i] = f2_copy.boxes[i];
    }

    frame_clean(f1_copy, threshold);

    //возвращаем копию
    return f1_copy;
}


int main() {
    frame f;
    f.boxes.resize(2);
    f.boxes[0] = box(1, 4, 4, 1, 0);
    f.boxes[1] = box(2, 3, 5, 1, 0);
    frame_clean(f, 1);
    std::cout << f.boxes.size() << std::endl;
    return 0;
}

