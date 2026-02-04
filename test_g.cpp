// Float to %g format (float only)
// Target: 100% match with printf("%g", float_value)

#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <vector>
#include <iostream>
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
    uint32_t truncated = (uint32_t)x;
    double fractional = x - truncated;
    
    if (fractional > 0.5) {
        return truncated + 1;
    } else if (fractional < 0.5) {
        return truncated;
    } else {
        // fractional == 0.5，向偶数舍入
        if (truncated % 2 == 0) {
            return truncated;
        } else {
            return truncated + 1;
        }
    }
}

inline uint64_t round_half_to_even64(double x) {
    uint64_t truncated = (uint64_t)x;
    double fractional = x - truncated;
    
    if (fractional > 0.5) {
        return truncated + 1;
    } else if (fractional < 0.5) {
        return truncated;
    } else {
        if (truncated % 2 == 0) {
            return truncated;
        } else {
            return truncated + 1;
        }
    }
}

// ============ Float to %g ============

inline int float_to_str_g(char* buf, float value) {
    if (value == 0.0f) {
        buf[0] = '0';
        return 1;
    }
    
    // 处理负数
    if (value < 0) {
        buf[0] = '-';
        return 1 + float_to_str_g(buf + 1, -value);
    }
    
    // 检查inf/nan
    uint32_t bits = *(uint32_t*)&value;
    uint32_t exp = (bits >> 23) & 0xFF;
    uint32_t mantissa = bits & 0x7FFFFF;
    
    if (exp == 255) {
        if (mantissa == 0) {
            memcpy(buf, "inf", 3);
            return 3;
        } else {
            memcpy(buf, "nan", 3);
            return 3;
        }
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
        
        // 银行家舍入到6位有效数字
        double factor = std::pow(10.0, 5);
        mantissa_val = round_half_to_even(mantissa_val * factor) / factor;
        
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
            
            int decimal = 5;
            double mult = std::pow(10.0, decimal);
            uint64_t frac_int = round_half_to_even64(frac * mult);
            
            if (frac_int >= (uint64_t)mult) {
                frac_int = 0;
                int_part++;
                result_len = uint32_to_str(result, int_part);
                memcpy(buf, result, result_len);
                buf[result_len++] = '.';
            }
            
            char frac_buf[16];
            int frac_len = uint32_to_str(frac_buf, (uint32_t)frac_int);
            
            for (int i = frac_len; i < decimal; i++) {
                buf[result_len++] = '0';
            }
            
            memcpy(buf + result_len, frac_buf, frac_len);
            result_len += frac_len;
            
            while (result_len > 0 && buf[result_len - 1] == '0') result_len--;
            if (result_len > 0 && buf[result_len - 1] == '.') result_len--;
        }
        
        // 指数
        buf[result_len++] = 'e';
        if (decade_exp >= 0) buf[result_len++] = '+';
        else {
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
            do {
                eb[el++] = '0' + (e % 10);
                e /= 10;
            } while (e > 0);
            while (el > 0) buf[result_len++] = eb[--el];
        }
        
    } else {
        // 普通格式
        int int_digits = decade_exp + 1;
        int need_frac = 6 - int_digits;
        
        if (need_frac <= 0) {
            // 银行家舍入到整数
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
            double mult = std::pow(10.0, need_frac);
            uint64_t frac_int = round_half_to_even64(frac * mult);
            
            // 处理进位
            if (frac_int >= (uint64_t)mult) {
                frac_int = 0;
                int_part++;
                result_len = uint32_to_str(result, int_part);
                memcpy(buf, result, result_len);
                buf[result_len++] = '.';
            }
            
            // 写入小数
            char frac_buf[16];
            int frac_len = uint32_to_str(frac_buf, (uint32_t)frac_int);
            
            for (int i = frac_len; i < need_frac; i++) {
                buf[result_len++] = '0';
            }
            
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
    char buf1[64], buf2[64];
    int match = 0, total = 0;
    
    printf("=== Float to %%g Format Verification ===\n\n");
    
    // 测试用例
    std::vector<float> test_vals = {
        0.0f, 0.1f, 0.01f, 0.001f, 0.0001f, 0.00001f, 0.000001f,
        1.0f, 10.0f, 100.0f, 1000.0f, 10000.0f, 100000.0f,
        1.1f, 1.11f, 1.111f, 1.1111f, 1.11111f, 1.111111f,
        10.1f, 10.11f, 10.111f,
        100.1f, 100.11f,
        1000.1f,
        1e-5f, 1e-4f, 1e-3f, 1e-2f, 1e-1f, 1e0f, 1e1f, 1e2f, 1e3f, 1e4f, 1e5f, 1e6f,
        0.123456f, 1.23456f, 12.3456f, 123.456f,
        3.14159f, 2.71828f,
    };
    
    printf("%-20s  %-20s  %-20s  %s\n", "Value", "float_to_str_g", "printf %g", "Match");
    printf("%-20s  %-20s  %-20s  %s\n", "-----", "----------------", "----------", "-----");
    
    for (float v : test_vals) {
        int len1 = float_to_str_g(buf1, v);
        buf1[len1] = '\0';
        std::snprintf(buf2, sizeof(buf2), "%g", (double)v);
        
        total++;
        bool ok = (strcmp(buf1, buf2) == 0);
        if (ok) match++;
        
        if (!ok) {
            printf("%-20.10g  %-20s  %-20s  ✗\n", (double)v, buf1, buf2);
        }
    }
    
    // 随机测试
    printf("\n--- Random Tests (10000 cases) ---\n");
    std::mt19937 rng(42);
    std::uniform_real_distribution<float> dist1(-1e6f, 1e6f);
    std::uniform_int_distribution<int> dist2(0, 3);
    
    int rand_match = 0, rand_total = 0;
    int fail_count = 0;
    
    for (int i = 0; i < 10000; i++) {
        float v;
        switch (dist2(rng)) {
            case 0: v = dist1(rng); break;
            case 1: v = (float)(rng() % 1000000); break;
            case 2: v = (float)(rng() % 1000 + 1) * std::pow(10.0f, (rng() % 20) - 10); break;
            default: v = (float)(rng() % 1000000) / 100.0f * std::pow(10.0f, (rng() % 10) - 5);
        }
        
        // 跳过inf/nan
        if (std::isinf(v) || std::isnan(v)) continue;
        
        if (v == 0) v = 0.000001f;
        if (v < 0) v = -v;
        
        int len1 = float_to_str_g(buf1, v);
        buf1[len1] = '\0';
        std::snprintf(buf2, sizeof(buf2), "%g", (double)v);
        
        rand_total++;
        if (strcmp(buf1, buf2) == 0) rand_match++;
        
        if (strcmp(buf1, buf2) != 0 && fail_count < 10) {
            printf("  ✗ %.10g -> spef='%s' printf='%s'\n", (double)v, buf1, buf2);
            fail_count++;
        }
    }
    
    printf("\n=== Result ===\n");
    printf("Hand-crafted: %d/%d match (%.2f%%)\n", match, total, 100.0 * match / total);
    printf("Random: %d/%d match (%.2f%%)\n", rand_match, rand_total, 100.0 * rand_match / rand_total);
    
    if (match == total && rand_match == rand_total) {
        printf("\n✓ All tests passed!\n");
        return 0;
    } else {
        printf("\n✗ Some tests failed.\n");
        return 1;
    }
}
