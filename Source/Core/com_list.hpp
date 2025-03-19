#ifndef _COM_LIST_HPP_
#define _COM_LIST_HPP_
#include <cstring>
#include <iterator>

#include "FreeRTOS.h"
#include "list.h"
#include "portable.h"
namespace ComList {
#define List_Index_Loop(item)                                     \
    for (ListItem_t *__end =                                      \
             (ListItem_t *)listGET_END_MARKER(item->pxContainer); \
         item != __end; item = item->pxNext)

template <typename _Type = int>
class list {
   public:
    list() { __handle = (List_t *)pvPortMalloc(sizeof(List_t)); }

    ~list() {
        ListItem_t *pxItem = __handle->xListEnd.pxNext;
        ListItem_t *pxNext;
        while ((void *)pxItem != (void *)(&(__handle->xListEnd))) {
            pxNext = pxItem->pxNext;
            if (pxItem->pxContainer != NULL) {
                /*this list item still in the list*/
                uxListRemove(pxItem);
            }
            if (pxItem->pvOwner != nullptr) {
                vPortFree(pxItem->pvOwner);
            }
            pxItem = pxNext;
        }
        vPortFree(__handle);
    }

    void push_back(const _Type &elm_to_push) {
        _Type *__elm = new _Type;
        ListItem_t *__item = new ListItem_t;
        std::memcpy(__elm, &elm_to_push, sizeof(_Type));

        vListInitialiseItem(__item);
        listSET_LIST_ITEM_OWNER(__item, __elm);

        vListInsertEnd(__handle, __item);
    }

    _Type &front() {
        return *(
            (_Type *)(((ListItem_t *)listGET_HEAD_ENTRY(__handle))->pvOwner));
    }

    _Type &back() {
        return *((_Type *)((__handle->xListEnd.pxPrevious->pvOwner)));
    }

    void pop_front() {
        ListItem_t *__item = (ListItem_t *)listGET_HEAD_ENTRY(__handle);
        uxListRemove(__item);
        if (__item->pvOwner != nullptr) {
            vPortFree(__item->pvOwner);
        }
        vPortFree(__item);
    }

    void pop_back() {
        ListItem_t *__item = __handle->xListEnd.pxPrevious;
        uxListRemove(__item);
        if (__item->pvOwner != nullptr) {
            vPortFree(__item->pvOwner);
        }
        vPortFree(__item);
    }

    void remove(const _Type &__elm) {
        ListItem_t *__item = (ListItem_t *)listGET_HEAD_ENTRY(__handle);
        ListItem_t *__next;
        List_Index_Loop(__item) {
            if (std::memcmp(&__elm, __item->pvOwner, sizeof(_Type)) == 0) {
                __next = (ListItem_t *)__item->pxNext;
                uxListRemove(__item);
                if (__item->pvOwner != nullptr) {
                    vPortFree(__item->pvOwner);
                }
                vPortFree(__item);
                __item = __next;
            }
        }
    }

    void print() {
        ListItem_t *pxItem = (ListItem_t *)(__handle->xListEnd.pxNext);
        printf("\r\n================================\r\n");
        printf("List:               0x%p\r\n", __handle);
        printf("Number of item:     %d\r\n", __handle->uxNumberOfItems);
        printf("xListEnd:           0x%p\r\n", &__handle->xListEnd);
        printf("xListEnd->pxNext:   0x%p\r\n", __handle->xListEnd.pxNext);

        uint8_t i = 0;
        List_Index_Loop(pxItem) {
            printf("\r\n--------------------------------\r\n");
            printf("order:          [%d]\r\n", i++);
            printf("pxItem:         0x%x\r\n", pxItem);
            printf("xItemValue:     %d\r\n", pxItem->xItemValue);
            printf("pxPrevious:     0x%p\r\n", pxItem->pxPrevious);
            printf("pxContainer:    0x%p\r\n", pxItem->pxContainer);
            printf("pvOwner:        0x%p\r\n", pxItem->pvOwner);
            printf("pxNext:         0x%p\r\n", pxItem->pxNext);
            // pxItem = pxItem->pxNext;
        }
        printf("\r\n================================\r\n");
    }

    uint32_t size() { return __handle->uxNumberOfItems; }

    class __list_iterator
        : public std::iterator<std::forward_iterator_tag, _Type> {
       private:
        ListItem_t *_current;

       public:
        __list_iterator(ListItem_t *item = nullptr) : _current(item) {}

        _Type &operator*() const {
            return *reinterpret_cast<_Type *>(_current->pvOwner);
        }

        _Type *operator->() const {
            return reinterpret_cast<_Type *>(_current->pvOwner);
        }

        __list_iterator &operator++() {
            _current = reinterpret_cast<ListItem_t *>(listGET_NEXT(_current));
            return *this;
        }

        __list_iterator operator++(int) {
            __list_iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        bool operator==(const __list_iterator &other) const {
            return _current == other._current;
        }

        bool operator!=(const __list_iterator &other) const {
            return _current != other._current;
        }
    };

    __list_iterator begin() const {
        return __list_iterator(
            reinterpret_cast<ListItem_t *>(listGET_HEAD_ENTRY(__handle)));
    }

    __list_iterator end() const {
        return __list_iterator(
            reinterpret_cast<ListItem_t *>(&(__handle->xListEnd)));
    }

   private:
    List_t *__handle;
};

}    // namespace BspList
#endif