#include <iostream>
#include <sstream>
#include <pthread.h>
#include <fstream>
#include <vector>
#include <unistd.h>
#include <sys/syscall.h>
#define gettid() syscall(SYS_gettid)
using namespace std;
struct data_term_frequency {
    string list_id;
    vector <int> list_num;
    double avg_cosine, cpu_time_used;
    int tid, i;
};
vector<string> list_text;
vector<data_term_frequency> data_case;
vector<bool> main_OK, thread_OK;
double sqrt_count(int num) {
    double temp = 1;
    while (abs(temp - num / temp) > 1e-5)
        temp = (temp + num / temp) / 2;
    return temp;
}
void process_structure(int error_letter, string temp) {
    if (!error_letter) {
        int i = 0;
        if (list_text.size() > 0) {
            while (temp != list_text[i]) {
                i++;
                if (i == list_text.size())
                    break;
            }
        }
        if (i == list_text.size()) {
            list_text.resize(list_text.size() + 1);
            list_text.back() = temp;            data_case.back().list_num.resize(data_case.back().list_num.size() + 1);
            data_case.back().list_num.back() = 1;
        }
        else
            data_case.back().list_num[i]++;
    }
}
int process_var(int num1, int num2) {
    int ans = 0;
    for (int i = 0; i < data_case[num1].list_num.size(); i++)
        ans = ans + data_case[num1].list_num[i] * data_case[num2].list_num[i];
    return ans;
}
void* print(void* sourse) {
    const clock_t begin = clock();
    data_term_frequency* data = (data_term_frequency*)sourse;
    data->tid = gettid();
    thread_OK[data->i] = 1;
    while (main_OK[data->i] == 0);
    int cosine_length = 0;
    double var_sum = 0, sum = 0, var = 0, results;
    for (int i = 0; i < data->list_num.size(); i++)
        var = var + data->list_num[i] * data->list_num[i];
    printf("[TID=%d] DocID:%s [", data->tid, data->list_id.c_str());
    for (int i = 0; i < data_case.back().list_num.size(); i++) {
        if (i < data->list_num.size())
            cout << data->list_num[i];
        else
            cout << 0;
        if (i < data_case.back().list_num.size() - 1)
            cout << ",";
        else
            cout << "]" << endl;
    }
    for (int i = 0; i < data_case.size(); i++) {
        if (data->list_id != data_case[i].list_id) {
            cosine_length = data->list_num.size() < data_case[i].list_num.size() ? data->list_num.size() : data_case[i].list_num.size();
            sum = 0;
            for (int j = 0; j < cosine_length; j++)
                sum = sum + data_case[i].list_num[j] * data->list_num[j];
            results = sum / sqrt_count(var * process_var(i, i));
            var_sum += results;
            printf("[TID=%d] cosine(%s,%s)=%.4g\n", data->tid, data->list_id.c_str(), data_case[i].list_id.c_str(), results);
        }
    }
    data->avg_cosine = var_sum / (data_case.size() - 1);
    printf("[TID=%d] Avg_cosine: %.4g\n[TID=%d] CPU time: %.4gms\n", data->tid, data->avg_cosine, data->tid, 1000 * (double)(clock() - begin) / CLOCKS_PER_SEC);
    pthread_exit(NULL);
}

int main(int aegc, char** argv) {
    clock_t begin = clock();
    ifstream inFile(argv[1]);
    if (!inFile) {
        cout << "Can't open file !" << endl;
        exit(1);
    }
    else {
        char byte;
        string temp = "";
        int sentence_num = 0, i = 0, j = 0;
        bool error_letter = false;
        while (inFile.get(byte)) {
            if (byte == '\n') {
                if (sentence_num % 2 && temp != "") { // ==1
                    process_structure(error_letter, temp);
                    error_letter = false;
                }
                else if (temp != "") {
                    data_case.resize(data_case.size() + 1);
                    if (data_case.size() != 1)
                        data_case.back().list_num.resize(list_text.size());
                    data_case.back().list_id = temp;
                }
                sentence_num++;
                temp = "";
            }
            else if ((byte <= 90 && byte >= 65) || (byte <= 122 && byte >= 97))
                temp += byte;
            else if (byte <= 57 && byte >= 48) {
                if (sentence_num % 2)
                    error_letter = true;
                temp += byte;
            }
            else {
                if (sentence_num % 2 && temp != "") { // ==1
                    process_structure(error_letter, temp);
                    temp = "";
                    error_letter = false;
                }
                else if (temp != "")
                    temp += byte;
            }
        }
        if (temp != "")
            process_structure(error_letter, temp);
        pthread_t tid[data_case.size()];
        main_OK.resize(data_case.size());
        thread_OK.resize(data_case.size());
        for (int i = 0; i < data_case.size(); ++i) {
            data_case[i].i = i;
        }
        for (int i = 0; i < data_case.size(); ++i) {
            pthread_create(&tid[i], NULL, print, (void*)&data_case[i]);
            while (thread_OK[i] == 0);
            printf("[Main thread]: create TID:%d, DocID:%s\n", data_case[i].tid, data_case[i].list_id.c_str());
            main_OK[i] = 1;
        }
        for (int i = 0; i < data_case.size(); i++)
            pthread_join(tid[i], NULL);
        int max_i;
        vector<int> list_i;
        double max = data_case[0].avg_cosine;
        list_i.resize(1);
        list_i[0] = i;
        for (i = 1; i < data_case.size(); i++) {
            if (data_case[i].avg_cosine > max) {
                max = data_case[i].avg_cosine;
                list_i.resize(1);
                list_i[0] = i;
            }
            else if (data_case[i].avg_cosine == max) {
                list_i.resize(list_i.size() + 1);
                list_i.back() = i;
            }
        }
        max_i = list_i[0];
        if (list_i.size() > 1) {
            for (i = 0; i < list_i.size(); i++) {
                if (stoi(data_case[max_i].list_id) > stoi(data_case[list_i[i]].list_id))
                    max_i = list_i[i];
            }
        }
        printf("[Main thread] KeyDocID:%s Highest Average Cosine: %.4g\n", data_case[max_i].list_id.c_str(), max);
    }
    printf("[Main thread] CPU time: %.4gms\n", 1000 * (double)(clock() - begin) / CLOCKS_PER_SEC);
}