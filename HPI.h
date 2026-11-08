/** 
 * @file HPI.h 
 * @version v4.1
*/
#ifndef HPI_H
#define HPI_H

#include <vector>
#include <cstring>
#include <string>
#include <iostream>
#include <iomanip>
#include <cstddef>
#include <type_traits>
#include <algorithm>
#include <stdexcept>
#include <stdio.h>

// Ensure cross platform optimization of bare pointers
// 确保裸指针优化的跨平台性
#if defined(_MSC_VER)
    #define HPI_RESTRICT __restrict
#elif defined(__GNUC__) || defined(__clang__)
    #define HPI_RESTRICT __restrict__
#else
    #define HPI_RESTRICT 
#endif

class HPI
{
    typedef unsigned long long ULL;
    typedef signed long long LL;
    // Based on 1e9 base
    // 基于 1e9 进制 
    static const int BASE = 1000000000; 
    // This is the startup threshold of Karatsuba algorithm, measured in limb. 
    // Users can customize this variable to achieve optimal performance on their own devices.
    // 此处为 Karatsuba 算法的启动阈值，单位是 limb，用户可以自定义该变量以在自己的设备上获取最佳性能
    static const int THRESHOLD = 128;

    private: // The following are private functions
             // 以下为私有函数

        // Core data storage container, data [0] is the sign bit, data [1] starts as the data, 
        // data [1] is the lowest bit, and the later it is, the higher the corresponding number of bits.
        // 核心数据存储容器，data[0]为符号位，data[1]开始为数据，data[1]为最低位，越往后对应位数越高
        std::vector<int> data; 
        // Length marker variable, records how many limbs were used in total for data storage.
        // 长度标记变量，记录数据存储总计用了多少个 limb
        ULL len; 

        // Core comparison function for six relational operators, provide directly call for their overloaded functions.
        // 六种关系运算符的核心比较函数, 供它们的运算符重载函数直接调用
        int compare(const HPI& other) const 
        {
            if(data[0] != other.data[0]) return data[0] ? -1 : 1;
            int cmp = absCompare(other);
            return data[0] ? -cmp : cmp;
        }
        
        // Absolute value comparison function.
        // 绝对值比较函数
        int absCompare(const HPI& other) const
        {
            if(len != other.len) return len < other.len ? -1 : 1;
            for(ULL i = len; i >= 1; i--) 
                if (data[i] != other.data[i]) return data[i] < other.data[i] ? -1 : 1;
            return 0;
        }

        // Absolute value addition.
        // 绝对值加
        HPI absAdd(const HPI& other) const
        {
            HPI c;
            c.len = std::max(len, other.len);
            c.data.assign(c.len + 2, 0);
            ULL minLen = std::min(len, other.len);
            int carry = 0;

            int* HPI_RESTRICT c_data = c.data.data();
            const int* HPI_RESTRICT a_data = data.data();
            const int* HPI_RESTRICT b_data = other.data.data();

            for(ULL i = 1; i <= minLen; i++)
            {
                int sum = a_data[i] + b_data[i] + carry;
                carry = (sum >= BASE);
                sum -= carry * BASE;
                c_data[i] = sum;
            }
            const int* HPI_RESTRICT maxOne = (len > other.len) ? a_data : b_data;
            for(ULL i = minLen + 1; i <= c.len; i++)
            {
                int sum = maxOne[i] + carry;
                carry = (sum >= BASE);
                sum -= carry * BASE;
                c_data[i] = sum;
            }
            if(carry) c_data[c.len++]++;

            return c;
        }

        // Absolute value subtraction.
        // 绝对值减
        HPI absSub(const HPI& other) const
        {
            HPI c;
            c.len = std::max(len, other.len);
            c.data.assign(c.len + 2, 0);
            ULL minLen = std::min(len, other.len);
            int carry = 0;

            int* HPI_RESTRICT c_data = c.data.data();
            const int* HPI_RESTRICT a_data = data.data();
            const int* HPI_RESTRICT b_data = other.data.data();

            for(ULL i = 1; i <= minLen; i++)
            {
                int sum = a_data[i] - b_data[i] - carry;
                carry = (sum < 0);
                sum += carry * BASE;
                c_data[i] = sum;
            }
            if(len > other.len)
                for(ULL i = minLen + 1; i <= c.len; i++)
                {
                    int sum = a_data[i] - carry;
                    carry = (sum < 0);
                    sum += carry * BASE;
                    c_data[i] = sum;
                }
            else
                for(ULL i = minLen + 1; i <= c.len && carry; i++)
                {
                    int sum = 0 - carry;
                    carry = (sum < 0);
                    sum += carry * BASE;
                    c_data[i] = sum;
                } 
            while(c.len && !c_data[c.len]) c.len--;
            if(!c.len) c_data[0] = 0;
            
            return c;
        }

        // Index based vertical multiplication.
        // 基于索引的竖式乘法
        HPI Schoolbook(const int* HPI_RESTRICT a_data, ULL a_len, const int* HPI_RESTRICT b_data, ULL b_len) const
        {
            if(a_len == 0 || b_len == 0) return HPI();
            
            HPI c;
            ULL carry = 0;
            c.len = a_len + b_len;
            c.data.assign(c.len + 2, 0);
        
            int* HPI_RESTRICT c_data = c.data.data();

            for(ULL i = 1; i <= a_len; i++)
            {
                ULL now = (ULL)a_data[i];
                for(ULL j = 1; j <= b_len; j++)
                {
                    ULL res = now * b_data[j] + c_data[i + j - 1] + carry;
                    c_data[i + j - 1] = res % BASE;
                    carry = res / BASE;
                }
                ULL carry_idx = i + b_len;
                while(carry)
                {
                    ULL res = carry + c_data[carry_idx];
                    c_data[carry_idx] = res % BASE;
                    carry = res / BASE;
                    carry_idx++;
                }
            }
            while(c.len && !c_data[c.len]) c.len--;
            if(!c.len) c.data[0] = 0;

            return c;
        }

        // Absolute value addition based on index, auxiliary function of Karatsuba algorithm.
        // 基于索引的绝对值加法，Karatsuba 算法的辅助函数
        HPI rangeAdd(const int* HPI_RESTRICT a_data, ULL a_len, const int* HPI_RESTRICT b_data, ULL b_len) const
        {
            HPI c;
            c.len = std::max(a_len, b_len);
            c.data.assign(c.len + 2, 0);
            ULL minLen = std::min(a_len, b_len);
            int carry = 0;

            int* HPI_RESTRICT c_data = c.data.data();

            for(ULL i = 1; i <= minLen; i++)
            {
                int sum = a_data[i] + b_data[i] + carry;
                carry = (sum >= BASE);
                sum -= carry * BASE;
                c_data[i] = sum;
            }
            const int* HPI_RESTRICT maxOne = (a_len > b_len) ? a_data : b_data;
            for(ULL i = minLen + 1; i <= c.len; i++)
            {
                int sum = maxOne[i] + carry;
                carry = (sum >= BASE);
                sum -= carry * BASE;
                c_data[i] = sum;
            }
            if(carry) c_data[++c.len]++;

            return c;
        }

        // Karatsuba algorithm core recursive function.
        // Karatsuba 算法核心递归函数
        HPI Karatsuba(const int* HPI_RESTRICT a_data, ULL a_len, const int* HPI_RESTRICT b_data, ULL b_len) const
        {
            if(a_len == 0 || b_len == 0) return HPI();
            else if(a_len <= THRESHOLD || b_len <= THRESHOLD) return Schoolbook(a_data, a_len, b_data, b_len);

            ULL n = std::max(a_len, b_len);
            ULL m = n / 2;
 
            const int* x0_data = a_data;
            ULL x0_len = std::min(a_len, m);
            const int* x1_data = a_data + m;
            ULL x1_len = (a_len >= m) ? (a_len - m) : 0;
            
            const int* y0_data = b_data;
            ULL y0_len = std::min(b_len, m);
            const int* y1_data = b_data + m;
            ULL y1_len = (b_len >= m) ? (b_len - m) : 0;

            HPI z2 = Karatsuba(x1_data, x1_len, y1_data, y1_len);
            HPI z0 = Karatsuba(x0_data, x0_len, y0_data, y0_len);
            HPI sum_a = rangeAdd(x0_data, x0_len, x1_data, x1_len);
            HPI sum_b = rangeAdd(y0_data, y0_len, y1_data, y1_len);
            HPI z1 = Karatsuba(sum_a.data.data(), sum_a.len, sum_b.data.data(), sum_b.len).absSub(z2).absSub(z0);

            if(z2.len) 
            {
                z2.data.resize(2 * n + 2, 0);
                std::memmove(&z2.data[2 * m + 1], &z2.data[1], z2.len * sizeof(int));
                std::memset(&z2.data[1], 0, 2 * m * sizeof(int));
                z2.len += 2 * m;
            }
            if(z1.len)
            {
                z1.data.resize(2 * n + 2, 0);
                std::memmove(&z1.data[m + 1], &z1.data[1], z1.len * sizeof(int));
                std::memset(&z1.data[1], 0, m * sizeof(int));
                z1.len += m;
            }
            
            z2 = z2.absAdd(z1).absAdd(z0);
            while(z2.len && !z2.data[z2.len]) z2.len--;
            
            return z2;
        }

        // High precision multiplied by low precision multiplication function, auxiliary functions for Knuth Algorithm D division.
        // 高精度乘低精度乘法函数，Knuth's Algorithm D 除法的辅助函数
        HPI smallMul(const HPI& a, int b) const
        {
            HPI c;
            c.len = a.len;
            c.data.assign(c.len + 2, 0);

            int* HPI_RESTRICT c_data = c.data.data();
            const int* HPI_RESTRICT a_data = a.data.data();

            ULL carry = 0;
            for(ULL i = 1; i <= a.len; i++)
            {
                ULL res = (ULL)a_data[i] * b + carry;
                c_data[i] = (int)(res % BASE);
                carry = res / BASE;
            }
            if(carry) c_data[++c.len] = (int)carry;
            while(!c_data[c.len] && c.len) c.len--;
            if(!c.len) c_data[0] = 0;

            return c;
        }

        // Knuth Algorithm D division core function.
        // Knuth's Algorithm D 除法核心函数
        void Knuth(const HPI& other, HPI& quo, HPI& rem) const
        {
            quo.len = len - other.len + 1;
            quo.data.assign(quo.len + 2, 0);
            rem.len = len;
            rem.data.assign(rem.len + 2, 0);

            if(other.len == 1) 
            {
                ULL div = other.data[1];
                ULL rem_cur = 0;

                int* HPI_RESTRICT q_data = quo.data.data();
                const int* HPI_RESTRICT u_data = data.data();
                
                for (ULL i = len; i >= 1; i--) 
                {
                    ULL cur = rem_cur * BASE + u_data[i];
                    q_data[i] = cur / div; 
                    rem_cur = cur % div;       
                }
                rem = rem_cur;
            } 
            else
            {
                HPI U = *this; HPI V = other;

                int d = BASE / (other.data[other.len] + 1);
                if(d > 1) { U = smallMul(U, d); V = smallMul(V, d); }
                U.data.push_back(0);
                U.len++;

                ULL v_len = V.len;
                ULL q_len = U.len - v_len;
                
                int* HPI_RESTRICT u_data = U.data.data();
                const int* HPI_RESTRICT v_data = V.data.data();
                int* HPI_RESTRICT q_data = quo.data.data();
                
                for(ULL i = q_len; i >= 1; i--) 
                {
                    ULL u0 = u_data[i + v_len];
                    ULL u1 = u_data[i + v_len - 1];
                    ULL u2 = (i + v_len >= 2) ? u_data[i + v_len - 2] : 0;
                    ULL v0 = v_data[v_len];
                    ULL v1 = v_data[v_len - 1];
                    ULL q_hat, r_hat;
                    
                    if(u0 == v0)
                    {
                        q_hat = BASE - 1;
                        r_hat = u1 + v0;
                    }
                    else
                    {
                        ULL num = u0 * BASE + u1;
                        q_hat = num / v0;
                        r_hat = num % v0;
                    }
                    while(q_hat >= BASE || (r_hat < BASE && q_hat * v1 > r_hat * BASE + u2))
                    {
                        q_hat--;
                        r_hat += v0;
                        if(r_hat >= BASE) break;
                    }

                    long long borrow = 0;
                    ULL carry = 0;
                    for (ULL k = 1; k <= v_len; k++) 
                    {
                        ULL prod = q_hat * v_data[k] + carry;
                        carry = prod / BASE;
                        long long sub = (long long)u_data[i + k - 1] - (prod % BASE) - borrow;
                        if (sub < 0) { sub += BASE; borrow = 1; } 
                        else borrow = 0;
                        u_data[i + k - 1] = (int)sub;
                    }
                    long long sub_final = (long long)u_data[i + v_len] - carry - borrow;
                    u_data[i + v_len] = (int)sub_final;

                    if (sub_final < 0) {
                        q_hat--;
                        int add_carry = 0;
                        for (ULL k = 1; k <= v_len; k++) 
                        {
                            int sum = u_data[i + k - 1] + v_data[k] + add_carry;
                            if (sum >= BASE) { sum -= BASE; add_carry = 1; } 
                            else add_carry = 0;
                            u_data[i + k - 1] = sum;
                        }
                        u_data[i + v_len] += add_carry;
                    }
                    
                    q_data[i] = (int)q_hat;
                }
                
                if(d > 1) 
                {
                    ULL rem_d = 0;
                    for (ULL i = v_len; i >= 1; i--) 
                    {
                        ULL cur = rem_d * BASE + u_data[i];
                        rem.data[i] = cur / d;
                        rem_d = cur % d;
                    }
                } 
                else 
                    for (ULL i = v_len; i >= 1; i--) rem.data[i] = u_data[i];
                rem.len = v_len;
            }
            
            while(quo.len && !quo.data[quo.len]) quo.len--; 
            if(!quo.len) quo.data[0] = 0;
            quo.data[0] = data[0] ^ other.data[0]; 

            while (rem.len && !rem.data[rem.len]) rem.len--;
            if (!rem.len) rem.data[0] = 0;
            rem.data[0] = data[0]; 
        }

        // The direct call function for overloading functions with addition operators and subtraction operator.
        // 加法与减法运算符重载函数的直接调用函数
        HPI HPIadd(const HPI& other, bool flip) const 
        {
            HPI c;
            bool otherSign = other.data[0] ^ flip;

            if(data[0] == otherSign)
            {
                c = absAdd(other);
                c.data[0] = data[0];
            }
            else
            {
                int cmp = absCompare(other);
                if(cmp >= 0)
                {
                    c = absSub(other);
                    c.data[0] = c.len ? data[0] : 0;
                } 
                else
                {
                    c = other.absSub(*this);
                    c.data[0] = c.len ? otherSign : 0;
                }
            }
            return c;
        }

        // The direct call function for overloading functions with multiplication operators.
        // 乘法运算符重载函数的直接调用函数
        HPI HPImul(const HPI& other) const 
        {
            HPI res = (len <= THRESHOLD || other.len <= THRESHOLD) 
                      ? Schoolbook(data.data(), len, other.data.data(), other.len)
                      : Karatsuba(data.data(), len, other.data.data(), other.len);
            res.data[0] = data[0] ^ other.data[0];
            return res;
        }

        // The direct calling function of the division operator overloaded function.
        // 除法运算符重载函数的直接调用函数
        HPI HPIdiv(const HPI& other) const 
        {
            if(!other.len) throw std::domain_error("Division by zero");
            if(absCompare(other) < 0) return HPI();

            HPI quo, rem;
            Knuth(other, quo, rem);

            return quo;
        }

        // The direct calling function of the modular operator overloaded function.
        // 取模运算符重载函数的直接调用函数
        HPI HPImod(const HPI& other) const
        {
            if(!other.len) throw std::domain_error("Modulo by zero");
            if(absCompare(other) < 0) return *this;

            HPI quo, rem;
            Knuth(other, quo, rem);

            return rem;
        }

        // The core function of constructing HPI objects using strings.
        // 使用字符串构造 HPI 对象的核心函数
        void stringBulid(const char* s, ULL i)
        {
            if(s == nullptr) throw std::invalid_argument("Null pointer");
            else if(!i) throw std::invalid_argument("Empty string");

            data.clear();
            data.push_back(0);
            data.reserve(i + 2);
            len = 0;

            ULL start = 0;
            while(start < i && s[start] == '0') start++;
            if (s[start] == '-') { data[0] = 1; start++; } 
            while(start < i && s[start] == '0') start++;
            while(i > start)
            {
                ULL j = (i >= start + 9) ? (i - 9) : start;

                ULL val = 0;
                for (ULL k = j; k < i; k++) 
                {
                    if(s[k] < 48 || s[k] > 57) throw std::invalid_argument("Input contains non-digit characters");
                    val = val * 10 + (s[k] - '0');
                }

                data.push_back((int)val);
                len++;

                i = j;
            }
            if(!len) data[0] = 0;
        }

        // Extended Euclidean Algorithm Functions.
        // 扩展欧几里得算法函数
        HPI exgcd(const HPI& a, const HPI& b, HPI& x, HPI& y) const
        {
            if (b == 0) {
                x = 1;
                y = 0;
                return a;
            }
            HPI x1, y1;
            HPI g = exgcd(b, a.HPImod(b), x1, y1);
            x = y1;
            y = x1.HPIadd(a.HPIdiv(b).HPImul(y1), true);
            return g;
        }

    public: // The following are the external interfaces 
            // 以下为对外接口

        // Constructor and Destructor and Overloaded assignment equal sign
        // 构造函数与析构函数与赋值等号重载

        HPI()
        {
            data.clear();
            data.push_back(0);
            len = 0; 
        }

        HPI(LL in) { *this = in; }

        HPI& operator=(const LL& in)
        {   
            data.clear();
            len = 0;
            ULL other;
            
            if(in < 0) { data.push_back(1); other = -in; }
            else { data.push_back(0); other = in; } 

            while(other)
            {
                len++;
                data.push_back(other % BASE);
                other /= BASE;
            }

            return *this;
        }

        HPI(const std::string& s) { stringBulid(s.c_str(), s.size()); }

        HPI& operator=(const std::string& s) { stringBulid(s.c_str(), s.size()); return *this; }

        HPI(HPI&&) noexcept = default;
        HPI& operator=(HPI&&) noexcept = default;

        HPI(const HPI&) = default;
        HPI& operator=(const HPI&) = default;

        ~HPI() = default; 

        // Accept arithmetic operator overload from another HPI object 
        // 接受另一个 HPI 对象的算术运算符重载

        HPI operator+(const HPI& other) const { return HPIadd(other, false); }
        HPI operator-(const HPI& other) const { return HPIadd(other, true); }
        /**
         * @brief High performance multiplication, 
         * automatically enable Karatsuba algorithm when both multiplier digits are higher than 1024 bits, 
         * And it uses bare pointer optimization, time complexity is O(n^1.585).
         * 高性能乘法，当两个乘数位数都高于 1024 位时，将自动启用 Karatsuba 算法，并且它使用了裸指针优化，时间复杂度为 O(n^1.585)
        */
        HPI operator*(const HPI& other) const { return HPImul(other); }
        /**
         * @brief High performance division,
         * when dividing by low-precision data, there is special processing, 
         * and the time complexity is O(n). When dividing by high-precision data, 
         * Knuth's Algorithm D division is automatically used, nd it will enable naked pointer optimization, 
         * the time complexity is O(n * m).
         * 高性能除法，当除以低精度数据时会有特化处理，时间复杂度为 O(n)，当除以高精度数据时，将自动启用 Knuth's Algorithm D 除法，
         * 并且会启用裸指针优化，时间复杂度为 O(n * m)
        */
        HPI operator/(const HPI& other) const { return HPIdiv(other); }
        /**
         * @brief High performance modular exponentiation,
         * when modulo a low-precision data with another data, there is special processing, 
         * and the time complexity is O(n). When dmodulo a high-precision data with another data, 
         * Knuth's Algorithm D division is automatically used, and it uses bare pointer optimization, 
         * the time complexity is O(n * m)
         * 高性能模运算，当对低精度数据进行取模时会有特化处理，时间复杂度为 O(n)，当对高精度数据进行取模时，会自动调用 Knuth 算法 D 进行除法，
         * 并且会启用裸指针优化，时间复杂度为 O(n * m)
         * @note The positive and negative of the modular exponentiation result depends on the modular number.
         * 取模结果的正负性取决于被取模数
        */
        HPI operator%(const HPI& other) const { return HPImod(other); }
        HPI operator-() const &
        {
            HPI res = *this;
            if(res.len) res.data[0] ^= 1;
            return res;
        }
        HPI operator-() && { if(len) data[0] ^= 1; return std::move(*this);; }
        explicit operator bool() const { return len > 0; }

        // Accept arithmetic operator overloading of native types 
        // 接受原生类型的算术运算符重载

        template <typename T>
        HPI operator+(const T& in) const
        {
            using U = std::remove_cv_t<std::remove_reference_t<T>>;
            static_assert(std::is_integral_v<U>, "Operand must be an integral or a HPI object");
            return HPIadd(HPI(in), false);
        }

        template <typename T>
        HPI operator-(const T& in) const
        {
            using U = std::remove_cv_t<std::remove_reference_t<T>>;
            static_assert(std::is_integral_v<U>, "Operand must be an integral type or a HPI object");
            return HPIadd(HPI(in), true);
        }

        /**
         * @brief High performance multiplication, 
         * automatically enable Karatsuba algorithm when both multiplier digits are higher than 1024 bits, 
         * And it uses bare pointer optimization, time complexity is O(n^1.585).
         * 高性能乘法，当两个乘数位数都高于 1024 位时，将自动启用 Karatsuba 算法，并且它使用了裸指针优化，时间复杂度为 O(n^1.585)
        */
        template <typename T>
        HPI operator*(const T& in) const
        {
            using U = std::remove_cv_t<std::remove_reference_t<T>>;
            static_assert(std::is_integral_v<U>, "Operand must be an integral type or a HPI object");
            return HPImul(HPI(in));
        }

        /**
         * @brief High performance division,
         * when dividing by low-precision data, there is special processing, 
         * and the time complexity is O(n). When dividing by high-precision data, 
         * Knuth's Algorithm D division is automatically used, nd it will enable naked pointer optimization, 
         * the time complexity is O(n * m).
         * 高性能除法，当除以低精度数据时会有特化处理，时间复杂度为 O(n)，当除以高精度数据时，将自动启用 Knuth's Algorithm D 除法，
         * 并且会启用裸指针优化，时间复杂度为 O(n * m)
        */
        template <typename T>
        HPI operator/(const T& in) const
        {
            using U = std::remove_cv_t<std::remove_reference_t<T>>;
            static_assert(std::is_integral_v<U>, "Operand must be an integral type or a HPI object");
            return HPIdiv(HPI(in));
        }

        /**
         * @brief High performance modular exponentiation,
         * when modulo a low-precision data with another data, there is special processing, 
         * and the time complexity is O(n). When dmodulo a high-precision data with another data, 
         * Knuth's Algorithm D division is automatically used, and it uses bare pointer optimization, 
         * the time complexity is O(n * m)
         * 高性能模运算，当对低精度数据进行取模时会有特化处理，时间复杂度为 O(n)，当对高精度数据进行取模时，会自动调用 Knuth 算法 D 进行除法，
         * 并且会启用裸指针优化，时间复杂度为 O(n * m)
         * @note The positive and negative of the modular exponentiation result depends on the modular number.
         * 取模结果的正负性取决于被取模数
        */
        template <typename T>
        HPI operator%(const T& in) const
        {
            using U = std::remove_cv_t<std::remove_reference_t<T>>;
            static_assert(std::is_integral_v<U>, "Operand must be an integral type or a HPI object");
            return HPImod(HPI(in));
        }

        // Reuse overloaded arithmetic operators to achieve post operation assignment
        // 复用重载过的算术运算符以实现运算后赋值

        template <typename T>
        HPI& operator+=(const T& other){ *this = *this + other; return *this; }
        template <typename T>
        HPI& operator-=(const T& other){ *this = *this - other; return *this; }
        template <typename T>
        HPI& operator*=(const T& other){ *this = *this * other; return *this; }
        template <typename T>
        HPI& operator/=(const T& other){ *this = *this / other; return *this; }
        template <typename T>
        HPI& operator%=(const T& other){ *this = *this % other; return *this; }
        HPI& operator++(){ *this = *this + 1; return *this; }
        HPI& operator--(){ *this = *this - 1; return *this; }
        HPI operator++(int) 
        {
            HPI old = *this;
            *this = *this + 1;
            return old;
        }
        HPI operator--(int) 
        {
            HPI old = *this;
            *this = *this - 1;
            return old;
        }

        // Accept relational operator overload from another HPI object
        // 接受另一个 HPI 对象的关系运算符重载

        bool operator<(const HPI& other) const { return compare(other) < 0; }
        bool operator<=(const HPI& other) const { return compare(other) <= 0; }
        bool operator>(const HPI& other) const { return compare(other) > 0; }
        bool operator>=(const HPI& other) const { return compare(other) >= 0; }
        bool operator==(const HPI& other) const { return compare(other) == 0; }
        bool operator!=(const HPI& other) const { return compare(other) != 0; }

        // Accept relational operator overloading of native types 
        // 接受原生类型的关系运算符重载

        template <typename T>
        bool operator<(const T& in) const 
        { 
            using U = std::remove_cv_t<std::remove_reference_t<T>>;
            static_assert(std::is_integral_v<U>, "Operand must be an integral type or a HPI object");
            return compare(HPI(in)) < 0; 
        }
        template <typename T>
        bool operator<=(const T& in) const 
        { 
            using U = std::remove_cv_t<std::remove_reference_t<T>>;
            static_assert(std::is_integral_v<U>, "Operand must be an integral type or a HPI object");
            return compare(HPI(in)) <= 0; 
        }
        template <typename T>
        bool operator>(const T& in) const 
        { 
            using U = std::remove_cv_t<std::remove_reference_t<T>>;
            static_assert(std::is_integral_v<U>, "Operand must be an integral type or a HPI object");
            return compare(HPI(in)) > 0; 
        }
        template <typename T>
        bool operator>=(const T& in) const 
        { 
            using U = std::remove_cv_t<std::remove_reference_t<T>>;
            static_assert(std::is_integral_v<U>, "Operand must be an integral type or a HPI object");
            return compare(HPI(in)) >= 0; 
        }
        template <typename T>
        bool operator==(const T& in) const 
        { 
            using U = std::remove_cv_t<std::remove_reference_t<T>>;
            static_assert(std::is_integral_v<U>, "Operand must be an integral type or a HPI object");
            return compare(HPI(in)) == 0; 
        }
        template <typename T>
        bool operator!=(const T& in) const 
        { 
            using U = std::remove_cv_t<std::remove_reference_t<T>>;
            static_assert(std::is_integral_v<U>, "Operand must be an integral type or a HPI object");
            return compare(HPI(in)) != 0; 
        }

        // Overloaded standard input-output stream operators
        // 重载的标准输入输出流运算符

        /**
         * @brief Overloads the stream insertion (>>) operator.
         * 重载的标准输入流（>>）运算符
         * @param os The input stream (e.g., std::cin).
         * 输入流对象
         * @param self The HPI object to input.
         * 需要输入的 HPI 对象
         * @return Reference to the input stream.
         * 返回输入流对象
        */
        friend std::istream& operator>>(std::istream& is, HPI& self)
        {
            std::string s;
            is >> s;
            self.stringBulid(s.c_str(), s.size());
            return is;
        }

        /**
         * @brief Overloads the stream insertion (<<) operator.
         * 重载的标准输出流运算符（<<）
         * @param os The output stream (e.g., std::cout).
         * 输出流对象
         * @param self The HPI object to print.
         * 需要打印的 HPI 对象
         * @return Reference to the output stream.
         * 返回输出流对象
         * @note Uses 9-digit zero-padding for intermediate limbs to maintain correct decimal formatting.
         * 中间的 limb 使用 9 位 0 填充以保证十进制格式的正确
        */
        friend std::ostream& operator<<(std::ostream& os, const HPI& self)
        {
            if(!self.len) os << 0;
            else
            {
                if(self.data[0]) os << '-';

                os << self.data[self.len];
                for (ULL i = self.len - 1; i >= 1; i--)
                    os << std::setw(9) << std::setfill('0') << self.data[i];
            }
            return os;
        }

        // Various tool functions
        // 各类工具函数

        /**
         * @brief Convert the lowest 18 decimal digits of the HPI objects to a signed long long. 
         * 将 HPI 对象的最低 18 位十进制数字转换为 long long 类型
         * @return The extracted value as a signed long long. If the HPI is negative, a negative long long is returned.
         * 返回一个 long long，如果 HPI 对象是负数，则返回值也会是负数
         * @warning If the absolute value of the HPI exceeds 18 decimal digits, the higher digits will be silently truncated.
         * 如果 HPI 对象的绝对值超过 18 位，则较高的数字将被静默截断
        */
        LL toLL() const
        {
            LL res = 0;
            ULL lim = std::min(len, (ULL)2);
            for(ULL i = lim; i >= 1; i--){ res = res * BASE + data[i]; }
            if(data[0]) res = -res;
            return res;
        }

        /**
         * @brief Convert the HPI objects to an arbitrary numeral system std::string.
         * 将 HPI 对象转换为一个任意进制的 std::string
         * @param base Specify the base, default is 10. 指定进制，默认为 10
         * @return The extracted value as a std::string. If the HPI is negative, The first character will be a negative sign.
         * 返回一个 std::string，如果 HPI 是负数，那么第一个字符将会是负号
         * @throw std::invalid_argument if base < 2 or base > 36.
         * 如果 base < 2 或者 base > 36 将会抛出 std::invalid_argument 错误
        */
        std::string toBaseStr(int base = 10) const
        {
            if(base < 2 || base > 36) throw std::invalid_argument("Base must be between 2 and 36");
            if(!len) return "0";

            std::string res;
            res.reserve(len * 3);
            HPI tmp = *this;
            tmp.data[0] = 0;
            const char digits[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
            int* HPI_RESTRICT tmp_data = tmp.data.data();
            
            while(tmp)
            {
                int rem_val = 0;

                for(int i = tmp.len; i >= 1; i--)
                {
                    ULL cur = (ULL)rem_val * BASE + tmp_data[i];
                    tmp_data[i] = cur / base;
                    rem_val = cur % base;
                }
                while(tmp.len && !tmp_data[tmp.len]) tmp.len--;

                res.push_back(digits[rem_val]);
            }
            if(data[0]) res.push_back('-');
            std::reverse(res.begin(), res.end());

            return res;
        }

        /**
         * @brief Get the decimal digits of the HPI object.
         * 获取 HPI 对象在十进制形式下的位数
         * @return An unsigned long long. 
         * 返回一个无符号 long long 类型
        */
        ULL getDigits() const
        {
            if(!len) return 1ULL;

            ULL digits = (len - 1) * 9;
            int higher = data[len];
            while(higher) 
            {
                digits++;
                higher /= 10;
            }
            
            return digits;
        }

        /**
         * @brief Specify slices for the absolute value of HPI objects.
         * 对 HPI 对象绝对值的指定切片
         * @param pos The starting position of the segment. Index starts with 0.
         * 切片的起始位置，索引从 0 开始
         * @param size The length of the segment to extract. 
         * 要提取的切片长度
         * @param startFromLowDigit If true, 'pos' counts from the lowest digit. 
         * If false (default), 'pos' counts from the highest digit. 
         * 如果为 true，则从最低位起算 pos，如果为 false（默认），则从最高位起算 pos
         * @return A new HPI object representing the extracted digits.
         * 返回一个包含截取数字的新 HPI 对象
         * @throws std::out_of_range If 'pos' is out of range or 'pos + size' exceeds the total number of digits.
         * 如果起始位置越界，或截取长度超出了大整数的总位数，将会抛出 std::out_of_range
         * @note The sign of the original number is ignored. If size is 0, it returns 0.
         * 此函数会忽略原数字的符号位。如果 size 为 0，则返回 0
        */
        HPI cutHPI(ULL pos, ULL size, bool startFromLowDigit = false) const
        {
            ULL decLen = getDigits();

            if(pos + size > decLen || pos >= decLen) throw std::out_of_range("Index out of bounds or invalid range");
            if(!size || !len) return HPI();
            
            ULL st = startFromLowDigit ? pos : decLen - pos - size;
            ULL ed = st + size - 1;
            ULL stLimbIdx = st / 9 + 1;
            ULL edLimbIdx = ed / 9 + 1;
            ULL stOff = st % 9;
            ULL edOff = ed % 9;

            std::string res;
            res.reserve(size);
            char buf[16];

            if(stLimbIdx == edLimbIdx)
            {
                std::snprintf(buf, sizeof(buf), "%09d", data[stLimbIdx]);
                res.append(buf + (8 - edOff), size);
            }
            else
            {
                std::snprintf(buf, sizeof(buf), "%09d", data[edLimbIdx]);
                res.append(buf + (8 - edOff), 1 + edOff);

                for (ULL i = edLimbIdx - 1; i > stLimbIdx; i--) 
                {
                    std::snprintf(buf, sizeof(buf), "%09d", data[i]);
                    res.append(buf, 9);
                }

                std::snprintf(buf, sizeof(buf), "%09d", data[stLimbIdx]);
                res.append(buf, 9 - stOff);
            }
            
            return HPI(res);
        }

        /**
         * @brief Get the absolute value of HPI object.
         * 获取 HPI 对象的绝对值
         * @return An HPI object.
         * 返回一个 HPI 对象
        */
        HPI abs() const
        {
            if(!data[0]) return *this;
            HPI res = *this;
            res.data[0] = 0;
            return res;
        }

        /**
         * @brief Power operation based on fast power algorithm.
         * 基于快速幂算法的幂运算
         * @param exp Accept long long, the exponent of power operation.
         * 接受 long long，作为幂运算的指数
         * @return An HPI object as the result of the operation.
         * 返回一个 HPI 对象作为运算结果
         * @throws std::invalid_argument if the exponent is negative.
         * 如果传入负指数将会抛出 std::invalid_argument 错误
        */
        HPI pow(long long exp) const
        {
            HPI base = *this;
            if(exp < 0) throw std::invalid_argument("Negative exponent");
            HPI res = 1;
            while(exp)
            {
                if(exp & 1) res = res.HPImul(base);
                base = base.HPImul(base);
                exp >>= 1;
            }
            return res;
        }

        /**
         * @brief Power operation based on fast power algorithm. And support modular exponentiation.
         * 支持取模的基于快速幂算法的幂运算
         * @param exp Accept long long, the exponent of power operation.
         * 接受 long long，作为幂运算的指数
         * @param mod Accept HPI, the modulus of power operation.
         * 接受 HPI，作为幂运算的模数
         * @return An HPI object as the result of the operation.
         * 返回一个 HPI 对象作为运算结果
         * @throws std::invalid_argument if the exponent is negative.
         * 如果传入负指数将会抛出 std::invalid_argument 错误
         * @note The positive and negative of the modulo result depends on the result of the power operation.
         * 取模结果的正负性取决于幂运算的结果
        */
        HPI pow(long long exp, const HPI& mod) const
        {
            if(exp < 0) throw std::invalid_argument("Negative exponent");
            if(mod == 0) throw std::domain_error("Modulo by zero");
            if(mod == 1) return HPI();
            HPI base = *this;
            HPI res = 1;
            while(exp)
            {
                if(exp & 1) res = res.HPImul(base).HPImod(mod);
                base = base.HPImul(base).HPImod(mod);
                exp >>= 1;
            }
            return res.HPImod(mod);
        }

        /**
         * @brief Calculate the square root of a large integer based on Newton's iterative method and round it down
         * 基于牛顿迭代法计算大整数的平方根并向下取整
         * @return An HPI object as the result of the operation.
         * 返回一个 HPI 对象作为运算结果
         * @throw std::domain_error if calculating the square root of a negative number.
         * 如果对负数开平方将会抛出 std::domain_error 错误
        */
        HPI isqrt() const
       {
            if(data[0]) throw std::domain_error("Calculate square root of a negative number");
            if(!len) return HPI();

            HPI x;
            x.len = len / 2 + 1;
            x.data.assign(x.len + 2, BASE - 1);

            HPI two = 2;

            while(true)
            {
                HPI next_x = x.absAdd(HPIdiv(x)).HPIdiv(two); 
                if(next_x.absCompare(x) >= 0) break;
                x = next_x;
            }
            return x;
       }
        
        /**
         * @brief Calculate the greatest common divisor of two HPI objects.
         * 计算两个 HPI 对象的最大公因数
         * @param other Another HPI object for performing operations. 进行运算的另一个 HPI 对象
         * @return An HPI object as the result of the operation.
         * 返回一个 HPI 对象作为运算结果
        */
        HPI gcd(HPI other) const
        {
            if(!len || !other.len) return HPI();
            HPI a = *this;
            a.data[0] = 0;
            while(other != 0) 
            {
                HPI tmp = a.HPImod(other);
                a = other;
                other = tmp;
            }
            return a;
        }
        
        /**
         * @brief Calculate the least common multiple of two HPI objects.
         * 计算两个 HPI 对象的最小公倍数
         * @param other Another HPI object for performing operations. 进行运算的另一个 HPI 对象
         * @return An HPI object as the result of the operation.
         * 返回一个 HPI 对象作为运算结果
        */
        HPI lcm(const HPI& other) const
        { 
            if(!len || !other.len) return HPI();
            HPI res = HPIdiv(gcd(other)).HPImul(other);
            res.data[0] = 0;
            return res;
        }

        /**
         * @brief Calculate the inverse element of HPI object under mod.
         * 计算 HPI 对象在模 mod 下的逆元
         * @param mod modulus. 模数
         * @return An HPI object as the result of the operation.
         * 返回一个 HPI 对象作为运算结果 
        */
        HPI modInv(const HPI& mod) const
        {
            HPI x, y;
            HPI a = HPImod(mod);
            if(a.data[0]) a = a.HPIadd(mod, false);

            HPI g = exgcd(a, mod, x, y);

            if(g != 1) throw std::domain_error("Modular inverse does not exist (not coprime)");
            else return x.HPImod(mod).HPIadd(mod, false).HPImod(mod);
        }

        /**
         * @brief Determine whether the HPI object is an even number. 判断 HPI 对象是否为偶数
         * @return A bool. 一个布尔值
        */
        bool isEven() const { return len ? data[1] % 2 == 0 : 1; }

        /**
         * @brief Determine whether the HPI object is an even number. 判断 HPI 对象是否为奇数
         * @return A bool. 一个布尔值
        */
        bool isOdd() const { return len ? data[1] % 2 == 1 : 0; }
};
#endif