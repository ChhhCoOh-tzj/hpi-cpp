/** 
 * @file HPI.h 
 * @version v4.0
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
    static const int BASE = 1000000000;
    static const int THRESHOLD = 128;

    private:
        std::vector<int> data;
        ULL len;

        int compare(const HPI& other) const 
        {
            if(data[0] != other.data[0]) return data[0] ? -1 : 1;

            int cmp = absCompare(other);

            return data[0] ? -cmp : cmp;
        }
        
        int absCompare(const HPI& other) const
        {
            if(len != other.len) return len < other.len ? -1 : 1;
            for(ULL i = len; i >= 1; i--) 
            {
                if (data[i] != other.data[i]) return data[i] < other.data[i] ? -1 : 1;
            }
            return 0;
        }

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

        HPI Schoolbook(const int* HPI_RESTRICT a_data, ULL a_len, 
                       const int* HPI_RESTRICT b_data, ULL b_len) const
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

        HPI rangeAdd(const int* HPI_RESTRICT a_data, ULL a_len, 
                     const int* HPI_RESTRICT b_data, ULL b_len) const
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

        HPI Karatsuba(const int* HPI_RESTRICT a_data, ULL a_len, 
                      const int* HPI_RESTRICT b_data, ULL b_len) const
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
                    ULL res = rem_cur * BASE + u_data[i];
                    q_data[i] = res / div; 
                    rem_cur = res % div;       
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
                    for (ULL k = 1; k <= v_len; k++) {
                        ULL prod = q_hat * v_data[k] + carry;
                        carry = prod / BASE;
                        long long sub = (long long)u_data[i + k - 1] - (prod % BASE) - borrow;
                        if (sub < 0) {
                            sub += BASE;
                            borrow = 1;
                        } else {
                            borrow = 0;
                        }
                        u_data[i + k - 1] = (int)sub;
                    }
                    long long sub_final = (long long)u_data[i + v_len] - carry - borrow;
                    u_data[i + v_len] = (int)sub_final;

                    if (sub_final < 0) {
                        q_hat--;
                        int add_carry = 0;
                        for (ULL k = 1; k <= v_len; k++) {
                            int sum = u_data[i + k - 1] + v_data[k] + add_carry;
                            if (sum >= BASE) {
                                sum -= BASE;
                                add_carry = 1;
                            } else {
                                add_carry = 0;
                            }
                            u_data[i + k - 1] = sum;
                        }
                        u_data[i + v_len] += add_carry;
                    }
                    
                    q_data[i] = (int)q_hat;
                }
                
                if (d > 1) 
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

        HPI HPImul(const HPI& other) const 
        {
            HPI res = (len <= THRESHOLD || other.len <= THRESHOLD) 
                      ? Schoolbook(data.data(), len, other.data.data(), other.len)
                      : Karatsuba(data.data(), len, other.data.data(), other.len);
            res.data[0] = data[0] ^ other.data[0];
            return res;
        }

        HPI HPIdiv(const HPI& other) const 
        {
            if(!other.len) throw std::domain_error("Division by zero");
            if(absCompare(other) < 0) return HPI();

            HPI quo, rem;
            Knuth(other, quo, rem);

            return quo;
        }

        HPI HPImod(const HPI& other) const
        {
            if(!other.len) throw std::domain_error("Modulo by zero");
            if(absCompare(other) < 0) return *this;

            HPI quo, rem;
            Knuth(other, quo, rem);

            return rem;
        }

        void stringBulid(const char* s, ULL i)
        {
            if(s == nullptr) throw std::invalid_argument("Null pointer");
            else if(!i) throw std::invalid_argument("Empty string");

            data.clear();
            data.push_back(0);
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

    public:
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

        HPI operator+(const HPI& other) const { return HPIadd(other, false); }
        HPI operator-(const HPI& other) const { return HPIadd(other, true); }
        /**
         * @brief High performance multiplication, 
         * automatically enable Karatsuba algorithm when both multiplier digits are higher than 1024 bits, 
         * And it uses bare pointer optimization, time complexity is O(n^1.585)
        */
        HPI operator*(const HPI& other) const { return HPImul(other); }
        /**
         * @brief High performance division,
         * when dividing by low-precision data, there is special processing, 
         * and the time complexity is O(n). When dividing by high-precision data, 
         * Knuth's Algorithm D division is automatically used, and it uses bare pointer optimization, 
         * the time complexity is O(n * m).
        */
        HPI operator/(const HPI& other) const { return HPIdiv(other); }
        /**
         * @brief High performance modular exponentiation,
         * when modulo a low-precision data with another data, there is special processing, 
         * and the time complexity is O(n). When dmodulo a high-precision data with another data, 
         * Knuth's Algorithm D division is automatically used, and it uses bare pointer optimization, 
         * the time complexity is O(n * m)
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
         * And it uses bare pointer optimization, time complexity is O(n^1.585)
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
         * Knuth's Algorithm D division is automatically used, and it uses bare pointer optimization, 
         * the time complexity is O(n * m).
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
        */
        template <typename T>
        HPI operator%(const T& in) const
        {
            using U = std::remove_cv_t<std::remove_reference_t<T>>;
            static_assert(std::is_integral_v<U>, "Operand must be an integral type or a HPI object");
            return HPImod(HPI(in));
        }

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

        bool operator<(const HPI& other) const { return compare(other) < 0; }
        bool operator<=(const HPI& other) const { return compare(other) <= 0; }
        bool operator>(const HPI& other) const { return compare(other) > 0; }
        bool operator>=(const HPI& other) const { return compare(other) >= 0; }
        bool operator==(const HPI& other) const { return compare(other) == 0; }
        bool operator!=(const HPI& other) const { return compare(other) != 0; }

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

        /**
         * @brief Overloads the stream insertion (>>) operator.
         * @param os The input stream (e.g., std::cin).
         * @param self The HPI object to input.
         * @return Reference to the input stream.
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
         * @param os The output stream (e.g., std::cout).
         * @param self The HPI object to print.
         * @return Reference to the output stream.
         * @note Uses 9-digit zero-padding for intermediate limbs to maintain correct decimal formatting.
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

        /**
         * @brief Convert the lowest 18 decimal digits of the HPI objects to a signed long long.
         * @return The extracted value as a signed long long. If the HPI is negative, a negative long long is returned.
         * @warning If the absolute value of the HPI exceeds 18 decimal digits, the higher digits will be silently truncated.
        */
        LL getLL()
        {
            LL res = 0;
            ULL lim = std::min(len, (ULL)2);
            for(ULL i = lim; i >= 1; i--){ res = res * BASE + data[i]; }
            if(data[0]) res = -res;
            return res;
        }

        /**
         * @brief Convert the HPI objects to a std::string.
         * @return The extracted value as a std::string. If the HPI is negative, The first character will be a negative sign.
        */
        std::string getStr()
        {
            if(!len) return "0";

            std::string res;
            char buf[16];
            res.reserve(len * 9 + 2);
            if(data[0]) res.push_back('-');
            
            int n = std::snprintf(buf, sizeof(buf), "%d", data[len]);
            res.append(buf, n);
            for (ULL i = len - 1; i >= 1; i--) {
                int n = std::snprintf(buf, sizeof(buf), "%09d", data[i]);
                res.append(buf, n);
            }

            return res;
        }

        /**
         * @brief Power operation based on fast power algorithm.
         * @param exp Accept long long, the exponent of power operation.
         * @return An HPI object as the result of the operation.
         * @throws std::invalid_argument if the exponent is negative.
        */
        HPI pow(long long exp)
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
         * @param exp Accept long long, the exponent of power operation.
         * @param mod Accept HPI, the modulus of power operation.
         * @return An HPI object as the result of the operation.
         * @throws std::invalid_argument if the exponent is negative.
        */
        HPI pow(long long exp, const HPI& mod)
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
         * @brief Get the absolute value of HPI object.
         * @return An HPI object.
        */
        HPI abs()
        {
            HPI res = *this;
            res.data[0] = 0;
            return res;
        }
};
#endif