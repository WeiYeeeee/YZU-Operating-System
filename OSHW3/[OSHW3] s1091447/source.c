#include <iostream>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <mutex>
#include <vector>
#include <iomanip>
using namespace std;
struct producer_data {
    int drone=0, producer_id;
};
struct dispatcher_data {
    int  dispatcher_id,component_num[3]; 
};
int i,waiting = 0,total_drone=0, features, component,producer_condition[3][3];
string component_name[3] = { "battery","aircraft", "propeller" };
vector<producer_data> producer_data_case;
vector<dispatcher_data> dispatcher_data_case;
pthread_mutex_t dispatcher_work, producer_work;
int check1(int num){
    bool error=true;
    for (i = 0; i < 3; i++) {
        if (producer_condition[i][num] == 0)
            error = false;
    }
    if (error) {
        num = rand() % 3;
        num = check1(num);
    }
    return num;
}
int check2(int num,int num1) {
    bool error = true;
    for (i = 0; i < 3; i++) {
        if (producer_condition[i][num] == 0)
            error = false;
    }
    if (error) {
        num = rand() % 2 + num1;
        num = check2(num, num1);
    }
    return num;
}
void* dispatcher(void* sourse) {
    dispatcher_data* data = (dispatcher_data*)sourse;
    while (total_drone < 50) {  
        if (!pthread_mutex_trylock(&dispatcher_work)) {
            if (waiting == 0 && total_drone < 50) {
                if (!features) {
                    component = rand() % 3;
                    component = check1(component);
                    cout << "Dispatcher: " << component_name[component] << endl;
                    data->component_num[component]++;
                    waiting = 1;
                }
                else {
                    bool error = true;
                    for (i = 0; i < 3; i++) {
                        if (producer_condition[i][0 + data->dispatcher_id] == 0)
                            error = false;
                        if (producer_condition[i][1 + data->dispatcher_id] == 0)
                            error = false;
                    }
                    if (!error) {
                        component = rand() % 2 + data->dispatcher_id;
                        component = check2(component, data->dispatcher_id);
                        cout << "Dispatcher" << char('A' + data->dispatcher_id) << ": " << component_name[component] << endl;
                        data->component_num[component]++;
                        waiting = 1;
                    }
                }
            }
            pthread_mutex_unlock(&dispatcher_work);
        }
    }
    pthread_exit(NULL);
}
void* producer(void* sourse) {
    producer_data* data = (producer_data*)sourse;
    int id=data->producer_id,work_success=0;
    while (total_drone < 50) {
        if (!pthread_mutex_trylock(&producer_work)) {
            if (waiting == 1) {
                if (component == 1 && id == 0)
                    work_success = 0;
                else {
                    if (producer_condition[id][component] == 0) {
                        producer_condition[id][component] = 1;
                        if (id == 0) {
                            cout << "Producer 1 (aircraft): get " << component_name[component] << endl;
                            if (producer_condition[0][0] == 1 && producer_condition[0][2] == 1) {
                                producer_condition[0][0] = 0;
                                producer_condition[0][2] = 0;
                                data->drone = data->drone + 1;
                                total_drone++;
                                cout << "Producer 1 (aircraft): OK, " << data->drone << " drone(s)\n";
                            }
                        }
                        else {
                            cout << "Producer " << 1 + data->producer_id << ": get " << component_name[component] << endl;
                            if (producer_condition[id][0] == 1 && producer_condition[id][1] == 1 && producer_condition[id][2] == 1) {
                                producer_condition[id][0] = 0;
                                producer_condition[id][1] = 0;
                                producer_condition[id][2] = 0;
                                data->drone = data->drone + 1;
                                total_drone++;
                                cout << "Producer " << 1 + data->producer_id << ": OK, " << data->drone << "drone(s)\n";
                            }
                        }
                        work_success = 1;
                    }
                    else
                        work_success = 0;
                }
                if (work_success == 1)
                    waiting = 0;
                else
                    waiting = 1;
            }
            pthread_mutex_unlock(&producer_work);
        }
    }
    pthread_exit(NULL);
}
int main(int aegc, char** argv) {
    features = stoi(argv[1]);
    int seed = stoi(argv[2]);
    if ((features == 1 || features == 0) && (seed >= 0 && seed <= 100)) {
    	producer_condition[0][1] = 1;
        srand(seed);
        dispatcher_data_case.resize(1 + features);
        producer_data_case.resize(3);
        for (i = 0; i <= features; i++)
            dispatcher_data_case[i].dispatcher_id = i;
        for (i = 0; i < 3; i++)
            producer_data_case[i].producer_id = i;
        pthread_mutex_init(&dispatcher_work, NULL);
        pthread_mutex_init(&producer_work, NULL);
        pthread_t threads[4 + features];
        for (i = 0; i <= features; i++)
            pthread_create(&threads[i], NULL, dispatcher, (void*)&dispatcher_data_case[i]);
        for (i = 0; i < 3; i++)
            pthread_create(&threads[i + 1 + features], NULL, producer, (void*)&producer_data_case[i]);
        for (int i = 0; i < 4 + features; i++)
            pthread_join(threads[i], NULL);
        if (features == 0) {
            cout << "\ndisptacher:\n";
            for (i = 0; i < 3; i++) {
                cout << " produce " << component_name[i] << ":";
                cout << setw(5 - i) << dispatcher_data_case[0].component_num[i] << " times\n";
            }
            for (i = 0; i < 3; i++) {
                cout << "\nproducer" << i + 1 << " produce " << setw(2) << producer_data_case[i].drone << "drones\n";
            }
        }
        else {
            for (i = 0; i < 2; i++) {
                cout << "\ndisptacher" << i + 1 << ":\n";
                for (int j = 0; j < 3; j++) {
                    cout << " produce " << component_name[j] << ":";
                    cout << setw(5 - j) << dispatcher_data_case[i].component_num[j] << " times\n";
                }
            }
            for (i = 0; i < 3; i++) {
                cout << "\nproducer" << i + 1 << " produce " << setw(2) << producer_data_case[i].drone << "drones\n";
            }
            pthread_mutex_destroy(&dispatcher_work);
            pthread_mutex_destroy(&producer_work);
        }
    }
    else
    	cout<<"input error\n";
}