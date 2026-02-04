// Float to %g format (positive only, no inf/nan)
// Target: 100% match with printf("%g", float_value)

#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <chrono>
#include <random>

// ============ Integer to string ============

inline int uint32_to_str(char* buf, uint32_t value) {
    if (value < 100) {
        static const char small_nums[100][3] = {
            "0","1","2","3","4","5","6","7","8","9","10","11","12","13","14","15",
            "16","17","18","19","20","21","22","23","24","25","26","27","28","29",
            "30","31","32","33","34","35","36","37","38","39","40","41","42","43",
            "44","45","46","47","48","49","50","51","52","53","54","55","56","57",
            "58","59","60","61","62","63","64","65","66","67","68","69","70","71",
            "72","73","74","75","76","77","78","79","80","81","82","83","84","85",
            "86","87","88","89","90","91","92","93","94","95","96","97","98","99"
        };
        const char* s = small_nums[value];
        int len = (int)strlen(s);
        memcpy(buf, s, len);
        return len;
    }
    
    char tmp[16];
    char* p = tmp;
    do {
        *p++ = '0' + (value % 10);
        value /= 10;
    } while (value > 0);
    
    int len = p - tmp;
    for (int i = 0; i < len; i++) {
        buf[i] = tmp[len - 1 - i];
    }
    return len;
}

// ============ 银行家舍入法 ============

inline uint32_t round_half_to_even(double x) {
    uint32_t t = (uint32_t)x;
    double f = x - t;
    if (f > 0.5) return t + 1;
    if (f < 0.5) return t;
    return (t % 2 == 0) ? t : t + 1;
}

inline uint64_t round_half_to_even64(double x) {
    uint64_t t = (uint64_t)x;
    double f = x - t;
    if (f > 0.5) return t + 1;
    if (f < 0.5) return t;
    return (t % 2 == 0) ? t : t + 1;
}

// ============ Float to %g ============

inline int float_to_str_g(char* buf, float value) {
    if (value == 0.0f) {
        buf[0] = '0';
        return 1;
    }
    
    // 计算10为底的对数
    double log10_val = std::log10(value);
    int decade_exp = (int)std::floor(log10_val + 1e-15);
    
    // 判断格式
    bool use_scientific = (decade_exp < -4 || decade_exp >= 6);
    
    char result[32];
    int result_len = 0;
    
    if (use_scientific) {
        // 科学计数法
        double mantissa_val = value / std::pow(10.0, decade_exp);
        mantissa_val = round_half_to_even(mantissa_val * 100000.0) / 100000.0;
        
        if (mantissa_val >= 10.0) {
            mantissa_val /= 10.0;
            decade_exp++;
        }
        
        // 整数部分
        uint32_t int_part = (uint32_t)mantissa_val;
        result_len = uint32_to_str(result, int_part);
        memcpy(buf, result, result_len);
        
        // 小数部分
        double frac = mantissa_val - int_part;
        if (frac > 1e-15) {
            buf[result_len++] = '.';
            uint64_t frac_int = round_half_to_even64(frac * 100000.0);
            
            if (frac_int >= 100000) {
                frac_int = 0;
                int_part++;
                result_len = uint32_to_str(result, int_part);
                memcpy(buf, result, result_len);
                buf[result_len++] = '.';
            }
            
            char frac_buf[16];
            int frac_len = uint32_to_str(frac_buf, (uint32_t)frac_int);
            for (int i = frac_len; i < 5; i++) buf[result_len++] = '0';
            memcpy(buf + result_len, frac_buf, frac_len);
            result_len += frac_len;
            
            while (result_len > 0 && buf[result_len - 1] == '0') result_len--;
            if (result_len > 0 && buf[result_len - 1] == '.') result_len--;
        }
        
        // 指数
        buf[result_len++] = 'e';
        if (decade_exp >= 0) {
            buf[result_len++] = '+';
        } else {
            buf[result_len++] = '-';
            decade_exp = -decade_exp;
        }
        if (decade_exp < 10) {
            buf[result_len++] = '0';
            buf[result_len++] = '0' + decade_exp;
        } else {
            char eb[8];
            int el = 0;
            int e = decade_exp;
            do { eb[el++] = '0' + (e % 10); e /= 10; } while (e > 0);
            while (el > 0) buf[result_len++] = eb[--el];
        }
        
    } else {
        // 普通格式
        int int_digits = decade_exp + 1;
        int need_frac = 6 - int_digits;
        
        if (need_frac <= 0) {
            // 四舍五入到整数
            uint32_t int_part = round_half_to_even(value);
            result_len = uint32_to_str(buf, int_part);
        } else {
            // 整数部分
            uint32_t int_part = (uint32_t)value;
            result_len = uint32_to_str(result, int_part);
            memcpy(buf, result, result_len);
            
            // 小数部分
            buf[result_len++] = '.';
            double frac = value - int_part;
            uint64_t frac_int = round_half_to_even64(frac * std::pow(10.0, need_frac));
            
            if (frac_int >= (uint64_t)std::pow(10.0, need_frac)) {
                frac_int = 0;
                int_part++;
                result_len = uint32_to_str(result, int_part);
                memcpy(buf, result, result_len);
                buf[result_len++] = '.';
            }
            
            char frac_buf[16];
            int frac_len = uint32_to_str(frac_buf, (uint32_t)frac_int);
            for (int i = frac_len; i < need_frac; i++) buf[result_len++] = '0';
            memcpy(buf + result_len, frac_buf, frac_len);
            result_len += frac_len;
            
            // 去掉尾部零
            while (result_len > 0 && buf[result_len - 1] == '0') result_len--;
            if (result_len > 0 && buf[result_len - 1] == '.') result_len--;
        }
    }
    
    return result_len;
}

// ============ Benchmark ============

int main() {
    const int COUNT = 1000000;
    char buf1[64], buf2[64];
    
    printf("=== Float to %%g Performance Benchmark ===\n\n");
    printf("Iterations: %d\n\n", COUNT);
    
    // 生成测试数据
    std::mt19937 rng(42);
    std::uniform_real_distribution<float> dist1(0.000001f, 1000000.0f);
    std::uniform_int_distribution<int> dist2(0, 3);
    std::vector<float> vals;
    vals.reserve(COUNT);
    
    for (int i = 0; i < COUNT; i++) {
        float v;
        switch (dist2(rng)) {
            case 0: v = dist1(rng); break;
            case 1: v = (float)(rng() % 1000000); break;
            case 2: v = (float)(rng() % 1000 + 1) * std::pow(10.0f, (rng() % 20) - 10); break;
            default: v = (float)(rng() % 1000000) / 100.0f * std::pow(10.0f, (rng() % 10) - 5);
        }
        // 跳过inf/nan
        if (std::isinf(v) || std::isnan(v)) { i--; continue; }
        if (v <= 0) { i--; continue; }
        vals.push_back(v);
    }
    
    int valid_count = vals.size();
    printf("Valid values: %zu\n\n", valid_count);
    
    // 验证正确性
    printf("Verifying correctness...\n");
    int correct = 0;
    for (float v : vals) {
        int len1 = float_to_str_g(buf1, v);
        buf1[len1] = '\0';
        std::snprintf(buf2, sizeof(buf2), "%g", (double)v);
        if (strcmp(buf1, buf2) == 0) correct++;
    }
    printf("Correct: %d/%d (%.2f%%)\n\n", correct, valid_count, 100.0 * correct / valid_count);
    
    // Benchmark printf
    printf("Benchmarking printf...\n");
    auto start = std::chrono::high_resolution_clock::now();
    for (float v : vals) {
        std::snprintf(buf1, sizeof(buf1), "%g", (double)v);
    }
    auto end = std::chrono::high_resolution_clock::now();
    double printf_ms = std::chrono::duration<double, std::milli>(end - start).count();
    
    // Benchmark float_to_str_g
    printf("Benchmarking float_to_str_g...\n");
    start = std::chrono::high_resolution_clock::now();
    for (float v : vals) {
        float_to_str_g(buf1, v);
    }
    end = std::chrono::high_resolution_clock::now();
    double custom_ms = std::chrono::duration<double, std::milli>(end - start).count();
    
    printf("\n=== Results ===\n");
    printf("printf:          %.2f ms\n", printf_ms);
    printf("float_to_str_g:  %.2f ms\n", custom_ms);
    printf("Speedup:         %.2fx\n", printf_ms / custom_ms);
    
    return 0;
}
